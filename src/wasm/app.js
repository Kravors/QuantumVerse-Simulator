/**
 * QuantumVerse WASM Portal - Frontend JavaScript
 * Connects HTML5 UI to the C++ WASM module via EMSCRIPTEN_BINDINGS.
 */

let Module = null;
let wasmBridge = null;
let discoveryInterval = null;
let isScanning = false;
let currentMass = 1.0;
let currentSpin = 0.0;
let currentCharge = 0.0;

// UI elements
const banner = document.getElementById('realityBanner');
const bannerText = document.getElementById('bannerText');
const canvas = document.getElementById('spacetimeCanvas');
const statusValid = document.getElementById('statusValid');
const statusInvalid = document.getElementById('statusInvalid');
const discoveryConsole = document.getElementById('discoveryConsole');
const coPilotText = document.getElementById('coPilotText');

// Slider elements
const sliderMass = document.getElementById('sliderMass');
const sliderSpin = document.getElementById('sliderSpin');
const sliderCharge = document.getElementById('sliderCharge');
const valMass = document.getElementById('valMass');
const valSpin = document.getElementById('valSpin');
const valCharge = document.getElementById('valCharge');
const dispMass = document.getElementById('dispMass');
const dispSpin = document.getElementById('dispSpin');
const dispCharge = document.getElementById('dispCharge');
const dispExtremality = document.getElementById('dispExtremality');

// Buttons
const btnValidate = document.getElementById('btnValidate');
const btnDiscovery = document.getElementById('btnDiscovery');
const btnStop = document.getElementById('btnStop');

// --- v6.0 Multi-User WebSocket Integration ---
let ws;
let isRemoteUpdate = false;
let myId = sessionStorage.getItem('qv_user_id') || 'User_' + Math.floor(Math.random() * 1000);
sessionStorage.setItem('qv_user_id', myId);
let reconnectDelay = 3000;
let reconnectAttempts = 0;

// Initialize
async function init() {
    if (!Module) {
        console.warn('WASM module not loaded yet');
        setTimeout(init, 100);
        return;
    }

    // Wait for WasmBridge to be available in the module
    if (!Module.WasmBridge) {
        console.warn('WasmBridge not ready yet');
        setTimeout(init, 100);
        return;
    }

    wasmBridge = new Module.WasmBridge();
    console.log('QuantumVerse WASM Bridge initialized');

    // Set up LLM callback
    wasmBridge.setLLMCallback((explanation) => {
        updateCoPilot(explanation);
        broadcastLLM(explanation);
    });

    // Initial validation
    validateState();

    // Set up UI event listeners
    setupUI();

    // Connect to shared spacetime relay
    connectToRelay();

    console.log('QuantumVerse WASM Portal ready');
}

function setupUI() {
    const sliderMass = document.getElementById('sliderMass');
    const sliderSpin = document.getElementById('sliderSpin');
    const sliderCharge = document.getElementById('sliderCharge');

    // Slider changes
    sliderMass.addEventListener('input', (e) => {
        console.log('[Slider] Mass input fired:', e.target.value);
        if (isRemoteUpdate) return;
        currentMass = parseFloat(e.target.value);
        valMass.textContent = currentMass.toFixed(1);
        dispMass.textContent = currentMass.toFixed(1);
        updateExtremality();

        // Update C++ state and broadcast to other clients
        if (wasmBridge) {
            wasmBridge.setMass(currentMass);
        }

        // Deform the WebGL grid immediately
        deformGrid(currentMass);

        broadcastState();
    });

    sliderSpin.addEventListener('input', (e) => {
        console.log('[Slider] Spin input fired:', e.target.value);
        if (isRemoteUpdate) return;
        currentSpin = parseFloat(e.target.value);
        valSpin.textContent = currentSpin.toFixed(2);
        dispSpin.textContent = currentSpin.toFixed(2);
        updateExtremality();

        // Update C++ state and broadcast to other clients
        if (wasmBridge) {
            wasmBridge.setSpin(currentSpin);
        }

        broadcastState();
    });

    sliderCharge.addEventListener('input', (e) => {
        console.log('[Slider] Charge input fired:', e.target.value);
        if (isRemoteUpdate) return;
        currentCharge = parseFloat(e.target.value);
        valCharge.textContent = currentCharge.toFixed(2);
        dispCharge.textContent = currentCharge.toFixed(2);
        updateExtremality();

        // Update C++ state and broadcast to other clients
        if (wasmBridge) {
            wasmBridge.setCharge(currentCharge);
        }

        broadcastState();
    });

    // Buttons
    btnValidate.addEventListener('click', validateState);

    btnDiscovery.addEventListener('click', () => {
        if (isScanning) return;
        isScanning = true;
        btnDiscovery.disabled = true;
        btnStop.disabled = false;
        logConsole('Starting RL Discovery Scan...', 'info');
        discoveryInterval = setInterval(runDiscoveryStep, 500);
    });

    btnStop.addEventListener('click', () => {
        isScanning = false;
        btnDiscovery.disabled = false;
        btnStop.disabled = true;
        if (discoveryInterval) {
            clearInterval(discoveryInterval);
            discoveryInterval = null;
        }
        logConsole('Discovery Scan stopped.', 'info');
    });
}

function updateExtremality() {
    const extremality = (currentSpin * currentSpin + currentCharge * currentCharge) /
        (currentMass * currentMass);
    dispExtremality.textContent = extremality.toFixed(4);
}

function logConsole(msg, type = 'info') {
    const entry = document.createElement('div');
    entry.className = `console-entry ${type}`;
    const time = new Date().toLocaleTimeString();
    entry.textContent = `[${time}] ${msg}`;
    discoveryConsole.appendChild(entry);
    discoveryConsole.scrollTop = discoveryConsole.scrollHeight;
}

function updateCoPilot(explanation) {
    coPilotText.textContent = explanation;
    coPilotText.scrollTop = coPilotText.scrollHeight;
}

function showAlert(show) {
    if (show) {
        banner.classList.remove('hidden');
        canvas.classList.add('alert');
        statusValid.classList.add('hidden');
        statusInvalid.classList.remove('hidden');
    } else {
        banner.classList.add('hidden');
        canvas.classList.remove('alert');
        statusValid.classList.remove('hidden');
        statusInvalid.classList.add('hidden');
    }
}

function validateState() {
    if (!wasmBridge) return;
    const result = JSON.parse(wasmBridge.validateState(currentMass, currentSpin, currentCharge));
    dispExtremality.textContent = ((currentSpin * currentSpin + currentCharge * currentCharge) /
        (currentMass * currentMass)).toFixed(4);

    if (result.valid) {
        showAlert(false);
        logConsole(`Validation passed (${result.failedChecks}/${result.totalChecks} checks). ${result.message}`, 'valid');
    } else {
        showAlert(true);
        logConsole(`VALIDATION FAILED: ${result.message}`, 'veto');
    }
}

async function runDiscoveryStep() {
    if (!wasmBridge || !isScanning) return;

    const result = JSON.parse(wasmBridge.runDiscoveryStep(currentMass, currentSpin, currentCharge));

    if (result.vetoed) {
        logConsole(`[AI VETO] ${result.reason}`, 'veto');
        logConsole(`Proposed state blocked: M=${currentMass.toFixed(2)} a=${currentSpin.toFixed(3)} Q=${currentCharge.toFixed(3)}`, 'info');
        // Don't update params on veto
    } else {
        // Apply deltas
        currentMass += result.deltaM;
        currentSpin += result.deltaA;
        currentCharge += result.deltaQ;

        // Clamp to slider ranges
        currentMass = Math.max(0.1, Math.min(100.0, currentMass));
        currentSpin = Math.max(0.0, Math.min(1.5, currentSpin));
        currentCharge = Math.max(0.0, Math.min(1.5, currentCharge));

        // Update sliders
        sliderMass.value = currentMass;
        sliderSpin.value = currentSpin;
        sliderCharge.value = currentCharge;
        valMass.textContent = currentMass.toFixed(1);
        valSpin.textContent = currentSpin.toFixed(2);
        valCharge.textContent = currentCharge.toFixed(2);
        dispMass.textContent = currentMass.toFixed(1);
        dispSpin.textContent = currentSpin.toFixed(2);
        dispCharge.textContent = currentCharge.toFixed(2);
        dispExtremality.textContent = result.extremality.toFixed(4);

        // Deform grid for AI-driven changes
        deformGrid(currentMass, currentSpin, currentCharge);

        logConsole(`[AI] M=${currentMass.toFixed(2)} a=${currentSpin.toFixed(3)} Q=${currentCharge.toFixed(3)} dM=${result.deltaM.toFixed(3)} da=${result.deltaA.toFixed(3)} dQ=${result.deltaQ.toFixed(3)} Extremality=${result.extremality.toFixed(4)}`, 'valid');
    }

    // Re-validate after step
    validateState();
}

// Expose init to be called when module loads
window.initWASM = init;

// --- v6.0 WebSocket Relay Functions ---

function connectToRelay() {
    if (ws && ws.readyState === WebSocket.OPEN) return;

    ws = new WebSocket('ws://localhost:8081');

    ws.onopen = () => {
        console.log('[WS] Connected to Shared Spacetime Relay');
        console.log('[WS] Connected as ' + myId);
        reconnectDelay = 3000;
        reconnectAttempts = 0;
    };

    ws.onmessage = (event) => {
        const msg = JSON.parse(event.data);

        if (msg.type === 'SYNC_STATE') {
            console.log(`[WS] Received from ${msg.state.lastUpdateBy} (me=${myId}): M=${msg.state.mass.toFixed(2)}`);

            if (msg.state.lastUpdateBy === myId) return;

            isRemoteUpdate = true;

            if (window.wasmBridge) {
                wasmBridge.setMass(msg.state.mass);
                wasmBridge.setSpin(msg.state.spin);
                wasmBridge.setCharge(msg.state.charge);
            }

            // Update UI sliders to match remote state
            sliderMass.value = msg.state.mass;
            sliderSpin.value = msg.state.spin;
            sliderCharge.value = msg.state.charge;
            valMass.textContent = msg.state.mass.toFixed(1);
            valSpin.textContent = msg.state.spin.toFixed(2);
            valCharge.textContent = msg.state.charge.toFixed(2);
            dispMass.textContent = msg.state.mass.toFixed(1);
            dispSpin.textContent = msg.state.spin.toFixed(2);
            dispCharge.textContent = msg.state.charge.toFixed(2);
            updateExtremality();

            currentMass = msg.state.mass;
            currentSpin = msg.state.spin;
            currentCharge = msg.state.charge;

            // Deform the WebGL grid to match the remote state
            deformGrid(msg.state.mass, msg.state.spin, msg.state.charge);

            isRemoteUpdate = false;
        }
        else if (msg.type === 'LLM_ANALYSIS') {
            document.getElementById('coPilotText').innerText = msg.text;
        }
    };

    ws.onerror = (err) => {
        console.error('[WS] Relay error:', err);
    };

    ws.onclose = () => {
        reconnectAttempts++;
        const delay = Math.min(3000 * Math.pow(2, reconnectAttempts - 1), 30000);
        console.log(`[WS] Disconnected from relay. Retrying in ${delay / 1000}s...`);
        setTimeout(connectToRelay, delay);
    };
}

let broadcastPending = null;

function broadcastState() {
    if (broadcastPending) return;
    broadcastPending = requestAnimationFrame(() => {
        broadcastPending = null;
        if (!ws) {
            console.warn('[WS] ws is null — not connected yet');
            return;
        }
        if (ws.readyState !== WebSocket.OPEN) {
            console.warn(`[WS] ws not open (readyState=${ws.readyState}), not broadcasting`);
            return;
        }
        if (isRemoteUpdate) return;

        const massSlider = document.getElementById('sliderMass') || document.getElementById('massSlider');
        const spinSlider = document.getElementById('sliderSpin');
        const chargeSlider = document.getElementById('sliderCharge');

        const mass = parseFloat(massSlider.value);
        const spin = parseFloat(spinSlider.value);
        const charge = parseFloat(chargeSlider.value);

        console.log(`[WS] Sending update: M=${mass.toFixed(2)}`);
        ws.send(JSON.stringify({
            type: 'UPDATE_STATE',
            mass: mass,
            spin: spin,
            charge: charge,
            user: myId
        }));
    });
}

function broadcastLLM(text) {
    if (!ws || ws.readyState !== WebSocket.OPEN) return;

    ws.send(JSON.stringify({
        type: 'LLM_ANALYSIS',
        text: text
    }));
}

// --- WebGL Spacetime Grid Renderer (Three.js) ---
let scene, camera, renderer, gridMesh, geometry;

function initWebGL() {
    try {
        const canvas = document.getElementById('spacetimeCanvas');
        if (!canvas) {
            console.error('[WebGL] Canvas #spacetimeCanvas not found in DOM');
            return;
        }
        if (typeof THREE === 'undefined') {
            console.error('[WebGL] THREE not loaded — check three.min.js CDN');
            return;
        }

        scene = new THREE.Scene();
        camera = new THREE.PerspectiveCamera(75, canvas.clientWidth / canvas.clientHeight, 0.1, 1000);
        camera.position.set(0, 25, 35);
        camera.lookAt(0, 0, 0);

        renderer = new THREE.WebGLRenderer({ canvas: canvas, antialias: true, alpha: true });
        renderer.setSize(canvas.clientWidth, canvas.clientHeight);
        renderer.setClearColor(0x05050a, 1); // Deep space black

        // Create a 64x64 grid
        geometry = new THREE.PlaneGeometry(50, 50, 64, 64);
        const material = new THREE.MeshBasicMaterial({
            color: 0x00ffcc,
            wireframe: true,
            side: THREE.DoubleSide
        });

        gridMesh = new THREE.Mesh(geometry, material);
        gridMesh.rotation.x = -Math.PI / 2; // Lay flat
        scene.add(gridMesh);

        console.log('[WebGL] Three.js initialized. Grid ready.');
        animate();
    } catch (e) {
        console.error('[WebGL] initWebGL failed:', e);
    }
}

function deformGrid(mass, spin = 0.0, charge = 0.0) {
    try {
        if (!geometry) return;
        const positions = geometry.attributes.position.array;
        const count = positions.length;

        // Scale the mass so 1.0 M☉ is a gentle dip, 100 M☉ is a deep well
        const massScale = Math.sqrt(mass) * 1.5;

        for (let i = 0; i < count; i += 3) {
            const x = positions[i];
            const y = positions[i + 1];

            const r = Math.sqrt(x * x + y * y);

            // Realistic 1/r potential (smooth funnel), clamped at center
            const depth = -massScale / (r * 0.1 + 1.0);
            positions[i + 2] = depth;
        }
        geometry.attributes.position.needsUpdate = true;
    } catch (e) {
        console.error('[WebGL] deformGrid failed:', e);
    }
}

function animate() {
    requestAnimationFrame(animate);
    if (renderer) renderer.render(scene, camera);
}

// Initialize WebGL when the DOM is ready (independent of WASM)
window.addEventListener('load', () => {
    console.log('[App] Window loaded, initializing WebGL...');
    setTimeout(initWebGL, 1000);
    connectToRelay();
});

// Bulletproof slider listener via event delegation — catches any range input on the page
document.addEventListener('input', (e) => {
    if (e.target.type !== 'range') return;

    const id = e.target.id || e.target.name || 'unknown';
    console.log(`[Slider] ${id} fired: ${e.target.value}`);

    const massSlider = document.getElementById('sliderMass') || document.getElementById('massSlider');
    const spinSlider = document.getElementById('sliderSpin');
    const chargeSlider = document.getElementById('sliderCharge');

    if (massSlider) {
        const mass = parseFloat(massSlider.value);
        const spin = parseFloat(spinSlider.value);
        const charge = parseFloat(chargeSlider.value);

        // Update module-level variables so broadcastState() sends fresh values
        currentMass = mass;
        currentSpin = spin;
        currentCharge = charge;

        // Update text labels below canvas
        valMass.textContent = mass.toFixed(1);
        valSpin.textContent = spin.toFixed(2);
        valCharge.textContent = charge.toFixed(2);
        dispMass.textContent = mass.toFixed(1);
        dispSpin.textContent = spin.toFixed(2);
        dispCharge.textContent = charge.toFixed(2);
        updateExtremality();

        // Deform the WebGL grid immediately
        deformGrid(mass, spin, charge);

        // Sync to WASM C++ core
        if (window.wasmBridge) {
            window.wasmBridge.setMass(mass);
            window.wasmBridge.setSpin(spin);
            window.wasmBridge.setCharge(charge);
        }

        // Broadcast to other browser windows
        broadcastState();
    }
});
