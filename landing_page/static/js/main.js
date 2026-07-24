import { animate, stagger, createScope, utils, engine } from 'https://cdn.jsdelivr.net/npm/animejs/+esm';
import * as THREE from 'three';
import { OBJLoader } from 'three/addons/loaders/OBJLoader.js';
import { MTLLoader } from 'three/addons/loaders/MTLLoader.js';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

// Some embedded/iframed preview contexts misreport document.visibilityState as
// hidden even while on-screen, which would otherwise freeze every animation
// at its first frame (anime.js pauses its engine on document hidden by default).
engine.pauseOnDocumentHidden = false;

// Safety net: entrance animations start elements at opacity:0 and rely on the
// rAF-driven engine to bring them to opacity:1. If the engine never gets a
// tick (throttled background tab, slow/blocked CDN, any JS hiccup), content
// must not stay permanently invisible — force the resting state after a beat.
setTimeout(() => {
  utils.set('[data-hero-line], [data-step-card], [data-feature]', {
    opacity: 1,
    translateY: 0,
  });
}, 2000);

/* =========================================================================
   Anime.js scope — entrance animation + scroll reveals, aware of reduced
   motion and viewport size via createScope's mediaQueries.
   https://animejs.com/documentation/scope
   ========================================================================= */
const pageScope = createScope({
  mediaQueries: {
    reduceMotion: '(prefers-reduced-motion: reduce)',
    isSmall: '(max-width: 860px)',
  },
}).add((self) => {
  const { reduceMotion, isSmall } = self.matches;

  // Hero entrance
  animate('[data-hero-line]', {
    opacity: [0, 1],
    translateY: reduceMotion ? [0, 0] : [16, 0],
    delay: stagger(reduceMotion ? 0 : 110, { start: 120 }),
    duration: reduceMotion ? 1 : 700,
    ease: 'outQuad',
  });

  animate('#hero-packet', {
    opacity: reduceMotion ? 0 : [0, 1, 1, 0],
    cx: reduceMotion ? 118 : [118, 118, 342, 342],
    duration: reduceMotion ? 1 : 2600,
    loop: true,
    delay: 1400,
    ease: 'inOutQuad',
  });

  // Scroll-triggered reveals for step cards + feature cards
  const revealTargets = [...document.querySelectorAll('[data-step-card], [data-feature]')];
  const io = new IntersectionObserver(
    (entries) => {
      entries.forEach((entry) => {
        if (!entry.isIntersecting) return;
        animate(entry.target, {
          opacity: [0, 1],
          translateY: reduceMotion ? [0, 0] : [20, 0],
          duration: reduceMotion ? 1 : 600,
          ease: 'outQuad',
        });
        io.unobserve(entry.target);
      });
    },
    { threshold: isSmall ? 0.1 : 0.2 }
  );
  revealTargets.forEach((el) => io.observe(el));

  return () => io.disconnect();
});

/* =========================================================================
   3D hardware viewer — three.js OBJ/MTL loader with orbit controls
   ========================================================================= */
function initViewer() {
  const stage = document.getElementById('viewer-stage');
  const loadingEl = document.getElementById('viewer-loading');
  const toggleBtn = document.getElementById('toggle-rotate');
  if (!stage) return;

  const reduceMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;

  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera(38, stage.clientWidth / stage.clientHeight, 0.1, 100);
  camera.position.set(2.4, 1.8, 2.6);

  const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  renderer.setSize(stage.clientWidth, stage.clientHeight);
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  stage.appendChild(renderer.domElement);

  scene.add(new THREE.AmbientLight(0xffffff, 1.1));
  const key = new THREE.DirectionalLight(0xffffff, 2.2);
  key.position.set(4, 6, 5);
  scene.add(key);
  const fill = new THREE.DirectionalLight(0xffffff, 0.8);
  fill.position.set(-5, 2, -4);
  scene.add(fill);

  const controls = new OrbitControls(camera, renderer.domElement);
  controls.enableDamping = true;
  controls.dampingFactor = 0.08;
  controls.autoRotate = !reduceMotion;
  controls.autoRotateSpeed = 2.2;
  controls.minDistance = 1;
  controls.maxDistance = 8;

  let modelGroup = null;

  new MTLLoader()
    .setPath('static/model/')
    .load('accelence.mtl', (materials) => {
      materials.preload();
      new OBJLoader()
        .setMaterials(materials)
        .setPath('static/model/')
        .load(
          'accelence.obj',
          (object) => {
            const box = new THREE.Box3().setFromObject(object);
            const center = box.getCenter(new THREE.Vector3());
            const size = box.getSize(new THREE.Vector3());
            const maxDim = Math.max(size.x, size.y, size.z) || 1;
            const scale = 1.6 / maxDim;

            object.position.sub(center);
            const group = new THREE.Group();
            group.add(object);
            group.scale.setScalar(scale);
            scene.add(group);
            modelGroup = group;

            loadingEl.style.opacity = '0';
            setTimeout(() => (loadingEl.style.display = 'none'), 300);

            if (!reduceMotion) {
              animate(group.scale, {
                x: [0, scale],
                y: [0, scale],
                z: [0, scale],
                duration: 900,
                ease: 'outElastic(1, .6)',
              });
            }
          },
          undefined,
          () => {
            loadingEl.innerHTML = '<span>Couldn’t load the 3D model.</span>';
          }
        );
    });

  function resize() {
    const w = stage.clientWidth;
    const h = stage.clientHeight;
    camera.aspect = w / h;
    camera.updateProjectionMatrix();
    renderer.setSize(w, h);
  }
  new ResizeObserver(resize).observe(stage);

  // Only run the render loop while the viewer is actually on screen —
  // no point spending a frame budget animating an off-screen canvas.
  let rafId = null;
  function renderFrame() {
    controls.update();
    renderer.render(scene, camera);
    rafId = requestAnimationFrame(renderFrame);
  }
  new IntersectionObserver(
    (entries) => {
      const visible = entries[0].isIntersecting;
      if (visible && rafId === null) renderFrame();
      if (!visible && rafId !== null) {
        cancelAnimationFrame(rafId);
        rafId = null;
      }
    },
    { threshold: 0.01 }
  ).observe(stage);

  toggleBtn?.addEventListener('click', () => {
    controls.autoRotate = !controls.autoRotate;
    toggleBtn.setAttribute('aria-pressed', String(controls.autoRotate));
  });
}
initViewer();

/* =========================================================================
   Interactive pseudocode segment — steps through one full lap across the
   sender and receiver boards, based on the algorithm in TEST1.ino / TEST2.ino
   ========================================================================= */
const senderLines = [
  'loop forever:',
  '  pulse the trigger pin for 10µs',
  '  duration = time for echo to return (30ms timeout)',
  '  distance_cm = duration × 0.034 ÷ 2',
  '',
  '  if 5cm ≤ distance_cm ≤ 75cm:        # runner in the gate zone',
  '    if not personAtGate:',
  '      if now − lastTriggerTime ≥ 1.5s:   # cooldown elapsed',
  '        personAtGate = true',
  '        lastTriggerTime = now',
  "        send '1' over Bluetooth   ───▶  # “runner detected”",
  '        wait 200ms',
  "        send '0' over Bluetooth",
  '  else:',
  '    personAtGate = false          # re-arm once runner clears the gate',
  '',
  '  wait 60ms   # let the ultrasonic echo settle',
];

const receiverLines = [
  'loop forever:',
  '  while Bluetooth has bytes waiting:',
  '    byte = read next byte',
  "    if byte == '1':                 ◀───── (from sender)",
  '      if now − lastStartSignalTime ≥ 1.5s:',
  '        lastStartSignalTime = now',
  '        if waitingForFinish:',
  '          # already timing a lap — ignore',
  '        else:',
  '          lapStartTime = now',
  '          waitingForFinish = true',
  '',
  '  pulse trigger pin, measure echo → distance_cm   # same as sender',
  '',
  '  if 5cm ≤ distance_cm ≤ 75cm:        # runner in the gate zone',
  '    if not personAtGate:',
  '      if now − lastTriggerTime ≥ 1.5s:   # cooldown elapsed',
  '        personAtGate = true',
  '        lastTriggerTime = now',
  '        if waitingForFinish:',
  '          lapTime = now − lapStartTime',
  '          lapNumber += 1',
  '          print "Lap", lapNumber, "completed in", lapTime, "sec"',
  '          waitingForFinish = false',
  '  else:',
  '    personAtGate = false          # re-arm once runner clears the gate',
  '',
  '  wait 60ms',
];

const steps = [
  {
    board: 'sender', lines: [0, 1, 2, 3],
    narrative: 'The sender gate pings its ultrasonic sensor roughly every 60ms and converts the echo time into a distance reading.',
    state: { distance: '118 cm', gate: false, waiting: false, lap: '—' },
  },
  {
    board: 'sender', lines: [5],
    narrative: 'A runner steps into the 5–75cm detection zone at the start line.',
    state: { distance: '38 cm', gate: false, waiting: false, lap: '—' },
  },
  {
    board: 'sender', lines: [6, 7, 8, 9],
    narrative: 'personAtGate was false and the 1.5s cooldown has elapsed, so this counts as a fresh arrival — the edge-trigger latch flips on.',
    state: { distance: '38 cm', gate: true, waiting: false, lap: '—' },
  },
  {
    board: 'sender', lines: [10, 11, 12], packet: 'toReceiver',
    narrative: "The sender fires '1' then '0' over the HC-05 Bluetooth link — the wireless equivalent of a wire pulsing HIGH then LOW.",
    state: { distance: '38 cm', gate: true, waiting: false, lap: '—' },
  },
  {
    board: 'receiver', lines: [1, 2, 3],
    narrative: "The receiver drains its Bluetooth buffer every pass through the loop and recognizes the '1' byte as a start signal.",
    state: { distance: '96 cm', gate: false, waiting: false, lap: '—' },
  },
  {
    board: 'receiver', lines: [4, 5, 8, 9, 10],
    narrative: 'No lap is currently running, so the receiver stamps lapStartTime and flips waitingForFinish on — the clock starts.',
    state: { distance: '96 cm', gate: false, waiting: true, lap: '—' },
  },
  {
    board: 'sender', lines: [13, 14],
    narrative: 'Back at the start line, the runner clears the 75cm boundary — personAtGate resets so the next runner can re-trigger the gate.',
    state: { distance: '118 cm', gate: false, waiting: true, lap: '—' },
  },
  {
    board: 'receiver', lines: [12, 14, 15, 16, 17, 18],
    narrative: "Moments later the same runner reaches the finish gate — the receiver's own sensor sees the identical 5–75cm crossing.",
    state: { distance: '41 cm', gate: true, waiting: true, lap: '—' },
  },
  {
    board: 'receiver', lines: [19, 20, 21, 22, 23], lap: true,
    narrative: 'Because waitingForFinish was true, this arrival closes the lap: lapTime is computed, the counter increments, and the result prints.',
    state: { distance: '41 cm', gate: true, waiting: false, lap: 'Lap 1 — 6.42 sec' },
  },
];

function renderPseudo(el, lines) {
  el.innerHTML = lines
    .map((text, i) => `<span class="line" data-line="${i}">${text.length ? text.replace(/</g, '&lt;') : '&nbsp;'}</span>`)
    .join('\n');
}

function initAlgorithm() {
  const codeSender = document.getElementById('code-sender');
  const codeReceiver = document.getElementById('code-receiver');
  const panelSender = document.getElementById('panel-sender');
  const panelReceiver = document.getElementById('panel-receiver');
  const narrativeEl = document.getElementById('narrative');
  const counterEl = document.getElementById('step-counter');
  const packetEl = document.getElementById('bt-packet');
  const stateDistance = document.getElementById('state-distance');
  const stateGate = document.getElementById('state-gate');
  const stateWaiting = document.getElementById('state-waiting');
  const stateLap = document.getElementById('state-lap');
  const btnPrev = document.getElementById('btn-prev');
  const btnNext = document.getElementById('btn-next');
  const btnPlay = document.getElementById('btn-play');
  const btnReset = document.getElementById('btn-reset');
  const iconPlay = document.getElementById('icon-play');
  const iconPause = document.getElementById('icon-pause');
  const tabButtons = [...document.querySelectorAll('.tabs [data-focus]')];

  if (!codeSender || !codeReceiver) return;

  renderPseudo(codeSender, senderLines);
  renderPseudo(codeReceiver, receiverLines);

  let index = 0;
  let playing = false;
  let timer = null;
  let focusMode = 'both';

  function applyFocus() {
    const dimSender = focusMode === 'receiver';
    const dimReceiver = focusMode === 'sender';
    panelSender.classList.toggle('dim', dimSender);
    panelReceiver.classList.toggle('dim', dimReceiver);
  }

  function paint() {
    const step = steps[index];
    counterEl.textContent = `Step ${index + 1} / ${steps.length}`;
    narrativeEl.textContent = step.narrative;

    [codeSender, codeReceiver].forEach((el) => {
      el.querySelectorAll('.line.active').forEach((l) => l.classList.remove('active'));
    });
    const targetCode = step.board === 'sender' ? codeSender : codeReceiver;
    step.lines.forEach((n) => targetCode.querySelector(`[data-line="${n}"]`)?.classList.add('active'));

    panelSender.classList.toggle('active-flash', step.board === 'sender');
    panelReceiver.classList.toggle('active-flash', step.board === 'receiver');

    stateDistance.textContent = step.state.distance;
    stateGate.textContent = String(step.state.gate);
    stateGate.classList.toggle('on', step.state.gate);
    stateWaiting.textContent = String(step.state.waiting);
    stateWaiting.classList.toggle('on', step.state.waiting);
    stateLap.textContent = step.state.lap;

    if (step.packet === 'toReceiver') {
      const line = packetEl.parentElement.getBoundingClientRect();
      const from = panelSender.getBoundingClientRect();
      const to = panelReceiver.getBoundingClientRect();
      const startX = from.left + from.width / 2 - line.left;
      const endX = to.left + to.width / 2 - line.left;
      utils.set(packetEl, { left: `${startX}px`, opacity: 0 });
      animate(packetEl, {
        left: [`${startX}px`, `${endX}px`],
        opacity: [0, 1, 1, 0],
        duration: 900,
        ease: 'inOutQuad',
      });
    }

    if (step.lap) {
      animate(stateLap, {
        scale: [1, 1.12, 1],
        duration: 500,
        ease: 'outQuad',
      });
    }
  }

  function goTo(i) {
    index = Math.max(0, Math.min(steps.length - 1, i));
    paint();
    if (index === steps.length - 1) stop();
  }

  function play() {
    if (playing) return;
    playing = true;
    iconPlay.style.display = 'none';
    iconPause.style.display = '';
    timer = setInterval(() => {
      if (index >= steps.length - 1) {
        stop();
        return;
      }
      goTo(index + 1);
    }, 1500);
  }

  function stop() {
    playing = false;
    iconPlay.style.display = '';
    iconPause.style.display = 'none';
    clearInterval(timer);
  }

  btnPrev.addEventListener('click', () => { stop(); goTo(index - 1); });
  btnNext.addEventListener('click', () => { stop(); goTo(index + 1); });
  btnReset.addEventListener('click', () => { stop(); goTo(0); });
  btnPlay.addEventListener('click', () => (playing ? stop() : play()));

  tabButtons.forEach((btn) => {
    btn.addEventListener('click', () => {
      tabButtons.forEach((b) => b.setAttribute('aria-selected', 'false'));
      btn.setAttribute('aria-selected', 'true');
      focusMode = btn.dataset.focus;
      applyFocus();
    });
  });

  paint();
}
initAlgorithm();
