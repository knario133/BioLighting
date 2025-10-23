// Move functions outside of DOMContentLoaded to make them global
let lang;
let translations;
let dom;
let testModeInterval = null;
let previewState = { r: 0, g: 0, b: 0, intensity: 100 };
const stages = {
    off:          { r: 0,   g: 0,   b: 0,   intensity: 0 },
    growth:       { r: 50,  g: 0,   b: 255, intensity: 100 },
    flowering:    { r: 255, g: 0,   b: 100, intensity: 100 },
    fullSpectrum: { r: 255, g: 255, b: 255, intensity: 100 },
    transition:   { r: 180, g: 0,   b: 180, intensity: 100 }
};
const testSequence = ['off', 'growth', 'transition', 'flowering', 'fullSpectrum'];


document.addEventListener('DOMContentLoaded', () => {
    // --- i18n Translations ---
    translations = {
        es: {
            title: "Control de Iluminación",
            labelR: "Rojo:",
            labelG: "Verde:",
            labelB: "Azul:",
            labelIntensity: "Intensidad:",
            actionsTitle: "Acciones",
            resetButton: "Resetear",
            wifiStatusLabel: "WiFi:",
            ipLabel: "IP:",
            connected: "Conectado",
            disconnected: "Desconectado",
            customColor: "Color Personalizado",
            stagesTitle: "Etapas",
            testMode: "Modo de Pruebas",
            stopTestMode: "Detener Pruebas",
            applyButton: "Aplicar Nuevo Color",
            btnOff: "Apagado",
            btnGrowth: "Crecimiento",
            btnFlowering: "Floración",
            btnFullSpectrum: "Completo",
            btnTransition: "Transición",
            confirmTitle: "Confirmar Acción",
            confirmText: "¿Estás seguro de que quieres aplicar este cambio?",
            confirmTestModeText: "¿Iniciar el modo de pruebas?",
            yes: "Sí",
            no: "No",
            "wifi.change": "Cambiar Wi-Fi",
            "wifi.scanning": "Buscando redes...",
            "wifi.select": "Selecciona una red",
            "wifi.password": "Contraseña",
            "wifi.connect": "Conectar",
            "wifi.retry": "Reintentar",
            "wifi.noneFound": "No se encontraron redes.",
            "wifi.success": "¡Conectado con éxito!",
            "wifi.error": "Error al conectar. Verifica la contraseña.",
            "wifi.verifying": "Verificando conexión...",
            "wifi.unreachable": "Dispositivo no accesible. Conéctate a la misma red y reintenta."
        },
        en: {
            title: "Lighting Control",
            labelR: "Red:",
            labelG: "Green:",
            labelB: "Blue:",
            labelIntensity: "Intensity:",
            actionsTitle: "Actions",
            resetButton: "Reset",
            wifiStatusLabel: "WiFi:",
            ipLabel: "IP:",
            connected: "Connected",
            disconnected: "Disconnected",
            customColor: "Custom Color",
            stagesTitle: "Stages",
            testMode: "Test Mode",
            stopTestMode: "Stop Tests",
            applyButton: "Apply New Color",
            btnOff: "Off",
            btnGrowth: "Growth",
            btnFlowering: "Flowering",
            btnFullSpectrum: "Full",
            btnTransition: "Transition",
            confirmTitle: "Confirm Action",
            confirmText: "Are you sure you want to apply this change?",
            confirmTestModeText: "Start test mode?",
            yes: "Yes",
            no: "No",
            "wifi.change": "Change Wi-Fi",
            "wifi.scanning": "Scanning for networks...",
            "wifi.select": "Select a network",
            "wifi.password": "Password",
            "wifi.connect": "Connect",
            "wifi.retry": "Retry",
            "wifi.noneFound": "No networks found.",
            "wifi.success": "Successfully connected!",
            "wifi.error": "Failed to connect. Check the password.",
            "wifi.verifying": "Verifying connection...",
            "wifi.unreachable": "Device unreachable. Connect to the same network and retry."
        }
    };

    // --- DOM Elements ---
    dom = {
        title: document.getElementById('title'),
        langSelect: document.getElementById('langSelect'),
        sliders: {
            r: document.getElementById('r-slider'),
            g: document.getElementById('g-slider'),
            b: document.getElementById('b-slider'),
            intensity: document.getElementById('intensity-slider')
        },
        inputs: {
            r: document.getElementById('r-input'),
            g: document.getElementById('g-input'),
            b: document.getElementById('b-input'),
        },
        values: { // Text values next to sliders (no longer used for RGB)
            intensity: document.getElementById('intensity-value')
        },
        labels: {
            r: document.getElementById('label-r'),
            g: document.getElementById('label-g'),
            b: document.getElementById('label-b'),
            intensity: document.getElementById('label-intensity'),
            colorPicker: document.getElementById('color-picker-label'),
            stages: document.getElementById('stages-title'),
        },
        actionsTitle: document.getElementById('actions-title'),
        resetButton: document.getElementById('reset-button'),
        wifiStatusLabel: document.getElementById('wifi-status-label'),
        wifiStatusValue: document.getElementById('wifi-status-value'),
        ipLabel: document.getElementById('ip-label'),
        ipValue: document.getElementById('ip-value'),
        colorPicker: document.getElementById('color-picker'),
        buttons: {
            off: document.getElementById('btn-off'),
            growth: document.getElementById('btn-growth'),
            flowering: document.getElementById('btn-flowering'),
            fullSpectrum: document.getElementById('btn-full-spectrum'),
            transition: document.getElementById('btn-transition'),
            testMode: document.getElementById('btn-test-mode'),
            apply: document.getElementById('btn-apply-color'),
            changeWifi: document.getElementById('btnChangeWifi'),
        }
    };

    lang = localStorage.getItem('lang') || document.documentElement.lang || 'es';

    // --- Event Listeners ---
    dom.langSelect.addEventListener('change', e => setLang(e.target.value));

    // Listen to Sliders
    ['r', 'g', 'b', 'intensity'].forEach(key => {
        dom.sliders[key].addEventListener('input', (e) => {
            stopTestMode();
            const value = parseInt(e.target.value, 10);
            updatePreviewControls({ [key]: value });
        });
    });

    // Listen to Number Inputs
    ['r', 'g', 'b'].forEach(key => {
        dom.inputs[key].addEventListener('input', (e) => {
            stopTestMode();
            let value = parseInt(e.target.value, 10);
            if (isNaN(value)) value = 0;
            if (value > 255) value = 255;
            if (value < 0) value = 0;
            updatePreviewControls({ [key]: value });
        });
    });

    // Listen to Color Picker
    dom.colorPicker.addEventListener('input', (e) => {
        stopTestMode();
        const rgb = hexToRgb(e.target.value);
        if (rgb) {
            updatePreviewControls(rgb);
        }
    });

    // Listen to Buttons that are NOT using onclick
    dom.buttons.apply.addEventListener('click', () => {
        confirmAndSetColor(previewState);
    });

    Object.keys(stages).forEach(stageName => {
        if (dom.buttons[stageName]) {
            dom.buttons[stageName].addEventListener('click', () => {
                const stageState = stages[stageName];
                updatePreviewControls(stageState);
                confirmAndSetColor(stageState);
            });
        }
    });

    // --- Initialization ---
    init();
});

// --- Language / i18n ---
function setLang(l) {
    lang = l;
    localStorage.setItem('lang', l);
    document.documentElement.lang = l;
    const t = translations[l];

    dom.title.textContent = t.title;
    dom.labels.r.textContent = t.labelR;
    dom.labels.g.textContent = t.labelG;
    dom.labels.b.textContent = t.labelB;
    dom.labels.intensity.textContent = t.labelIntensity;
    dom.labels.stages.textContent = t.stagesTitle;
    dom.labels.colorPicker.textContent = t.customColor;
    dom.actionsTitle.textContent = t.actionsTitle;
    dom.resetButton.textContent = t.resetButton;
    dom.buttons.apply.textContent = t.applyButton;
    dom.wifiStatusLabel.textContent = t.wifiStatusLabel;
    dom.ipLabel.textContent = t.ipLabel;
    dom.buttons.testMode.textContent = testModeInterval ? t.stopTestMode : t.testMode;

    // Update stage buttons
    dom.buttons.off.textContent = t.btnOff;
    dom.buttons.growth.textContent = t.btnGrowth;
    dom.buttons.flowering.textContent = t.btnFlowering;
    dom.buttons.fullSpectrum.textContent = t.btnFullSpectrum;
    dom.buttons.transition.textContent = t.btnTransition;
    document.querySelector('[data-i18n="wifi.change"]').textContent = t["wifi.change"];
}

// --- API Communication ---
async function fetchWithTimeout(resource, options = {}) {
    const { timeout = 8000 } = options;
    const controller = new AbortController();
    const id = setTimeout(() => controller.abort(), timeout);

    const response = await fetch(resource, {
        ...options,
        signal: controller.signal,
        cache: 'no-store'
    });
    clearTimeout(id);
    return response;
}

const confirmAndSetColor = (state, text = null) => {
    const t = translations[lang];
    const confirmText = text || t.confirmText;

    Swal.fire({
        title: t.confirmTitle,
        text: confirmText,
        icon: 'question',
        showCancelButton: true,
        confirmButtonText: t.yes,
        cancelButtonText: t.no,
        confirmButtonColor: '#3085d6',
        cancelButtonColor: '#6b7280'
    }).then(result => {
        if (result.isConfirmed) {
            setColor(state);
        }
    });
}

const setColor = async (state) => {
    stopTestMode();
    try {
        const response = await fetch('/api/light', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(state)
        });
        if (response.ok) {
            const newState = await response.json();
            updateUi(newState); // Sync UI with the confirmed state from device
        }
    } catch (error) {
        console.error('Failed to update light state:', error);
    }
};

const fetchLightState = async () => {
    try {
        const response = await fetch('/api/light');
        if (response.ok) {
            const state = await response.json();
            updateUi(state);
        }
    } catch (error) {
        console.error('Failed to fetch light state:', error);
    }
};

const fetchWifiState = async () => {
    try {
        const response = await fetch('/api/wifi/status');
        if (response.ok) {
            const state = await response.json();
            updateWifiUi(state);
        }
    } catch (error) {
        console.error('Failed to fetch wifi state:', error);
    }
};


const toHex = (c) => `0${(c || 0).toString(16)}`.slice(-2);

// This function updates ONLY the UI controls from a state object
const updatePreviewControls = (state) => {
    previewState = { ...previewState, ...state };

    dom.sliders.r.value = previewState.r;
    dom.inputs.r.value = previewState.r;

    dom.sliders.g.value = previewState.g;
    dom.inputs.g.value = previewState.g;

    dom.sliders.b.value = previewState.b;
    dom.inputs.b.value = previewState.b;

    dom.sliders.intensity.value = previewState.intensity;
    dom.values.intensity.textContent = `${previewState.intensity}%`;

    dom.colorPicker.value = `#${toHex(previewState.r)}${toHex(previewState.g)}${toHex(previewState.b)}`;
};

const updateUi = (state) => {
    previewState = state;
    updatePreviewControls(state);
};

const updateWifiUi = (state) => {
    const t = translations[lang];
    dom.wifiStatusValue.textContent = state.wifi ? `${t.connected} (${state.ssid})` : t.disconnected;
    dom.ipValue.textContent = state.ip;
};

const hexToRgb = (hex) => {
    const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16)
    } : null;
};

// --- Global Event Handlers for onclick ---
window.openWifiModal = async () => {
    // This will handle polling for scan results
    const pollForScanResults = async (pollController) => {
        let attempts = 0;
        const maxAttempts = 15; // 15 attempts * 2s delay = 30s timeout

        while (attempts < maxAttempts) {
            if (pollController.signal.aborted) throw new Error('Polling for scan results aborted');

            try {
                const response = await fetchWithTimeout('/api/wifi/results', { signal: pollController.signal, timeout: 5000 });

                if (response.status === 200) {
                    const networks = await response.json();
                    return networks; // Success, return the list
                } else if (response.status === 202) {
                    await new Promise(resolve => setTimeout(resolve, 2000));
                    attempts++;
                } else {
                    throw new Error(`Unexpected status: ${response.status}`);
                }
            } catch (error) {
                console.error(`Scan poll attempt ${attempts + 1} failed:`, error);
                throw new Error("Polling for scan results failed");
            }
        }
        throw new Error("Polling for scan results timed out");
    };

    const pollForConnectionStatus = async (pollController) => {
        let attempts = 0;
        const backoff = [2000, 5000, 10000];

        const checkStatus = async () => {
            if (pollController.signal.aborted) throw new Error('Polling aborted');

            try {
                const response = await fetchWithTimeout('/api/wifi/status', { signal: pollController.signal });
                const data = await response.json();

                if (data.mode === 'STA' && data.status === 'connected') return true;

                attempts++;
                const delay = backoff[Math.min(attempts - 1, backoff.length - 1)] || 30000;
                await new Promise(resolve => setTimeout(resolve, delay));
                return checkStatus();

            } catch (error) {
                console.error(`Connection poll attempt ${attempts + 1} failed:`, error);
                attempts++;
                if (attempts >= 3) throw new Error("Polling failed after 3 attempts");

                const delay = backoff[Math.min(attempts - 1, backoff.length - 1)] || 30000;
                await new Promise(resolve => setTimeout(resolve, delay));
                return checkStatus();
            }
        };
        return checkStatus();
    };

    const unreachableStep = () => {
        let pollAbortController = new AbortController();

        return Swal.fire({
            icon: 'error',
            text: translations[lang]['wifi.unreachable'],
            confirmButtonText: translations[lang]['wifi.retry'],
            didOpen: () => {
                Swal.showLoading();
                pollForConnectionStatus(pollAbortController)
                    .then(result => { if (result) Swal.fire({ icon: 'success', text: translations[lang]['wifi.success'] }) })
                    .catch(() => { Swal.hideLoading() });
            }
        });
    };

    Swal.fire({
        title: translations[lang]['wifi.change'],
        html: `<div>${translations[lang]['wifi.scanning']}</div>`,
        allowOutsideClick: false,
        allowEscapeKey: false,
        didOpen: async () => {
            Swal.showLoading();
            let pollScanController = new AbortController();

            try {
                const scanInitiateResponse = await fetchWithTimeout('/api/wifi/scan');
                if (scanInitiateResponse.status !== 202) {
                    throw new Error("Failed to initiate scan");
                }

                const networks = await pollForScanResults(pollScanController);

                if (networks && networks.length > 0) {
                    const options = networks.map(net => `<option value="${net.ssid}">${net.ssid} (${net.rssi} dBm)</option>`).join('');
                    Swal.update({
                        html: `
                            <select id="ssidSelect" class="swal2-select">${options}</select>
                            <input type="password" id="wifiPwd" class="swal2-input" placeholder="${translations[lang]['wifi.password']}">
                        `,
                        showConfirmButton: true,
                        confirmButtonText: translations[lang]['wifi.connect'],
                    });
                } else {
                    Swal.update({
                        html: translations[lang]['wifi.noneFound'],
                        confirmButtonText: translations[lang]['wifi.retry'],
                        showConfirmButton: true,
                        preConfirm: () => false
                    });
                }
            } catch (error) {
                console.error("WiFi Scan process failed:", error);
                Swal.update({
                    html: translations[lang]['wifi.error'],
                    confirmButtonText: translations[lang]['wifi.retry'],
                    showConfirmButton: true,
                    preConfirm: () => false
                });
            }
        },
        preConfirm: async () => {
            const ssid = document.getElementById('ssidSelect')?.value;
            const password = document.getElementById('wifiPwd')?.value;

            if (!ssid) {
                openWifiModal();
                return false;
            }

            const credentials = { ssid, password };

            try {
                const response = await fetchWithTimeout('/api/wifi/connect', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(credentials)
                });
                if (!response.ok) throw new Error('Connect request failed');

                Swal.update({
                    title: translations[lang]['wifi.verifying'],
                    html: '',
                    showConfirmButton: false,
                    showLoaderOnConfirm: true
                });
                Swal.showLoading();

                let pollConnectController = new AbortController();
                await pollForConnectionStatus(pollConnectController);
                return 'connected';

            } catch (error) {
                console.error("Connect or poll failed:", error);
                if (error.message.includes("Polling failed")) return 'unreachable';

                Swal.showValidationMessage(translations[lang]['wifi.error']);
                return false;
            }
        }
    }).then((result) => {
        if (result.isConfirmed) {
            if (result.value === 'connected') {
            Swal.fire({ icon: 'success', text: translations[lang]['wifi.success'] });
            } else if (result.value === 'unreachable') {
            unreachableStep();
            }
        }
    });
}

window.handleTestModeClick = () => {
    if (testModeInterval) {
        stopTestMode();
    } else {
        const t = translations[lang];
        Swal.fire({
            title: t.confirmTitle,
            text: t.confirmTestModeText,
            icon: 'question',
            showCancelButton: true,
            confirmButtonText: t.yes,
            cancelButtonText: t.no,
        }).then(result => {
            if (result.isConfirmed) {
                startTestMode();
            }
        });
    }
};

window.handleResetClick = () => {
    confirmAndSetColor({ r: 0, g: 0, b: 0, intensity: 0 });
};

const stopTestMode = () => {
    if (testModeInterval) {
        clearInterval(testModeInterval);
        testModeInterval = null;
        dom.buttons.testMode.textContent = translations[lang].testMode;
    }
};

const startTestMode = () => {
    if (testModeInterval) return;

    dom.buttons.testMode.textContent = translations[lang].stopTestMode;
    let currentStep = 0;

    testModeInterval = setInterval(() => {
        const stageName = testSequence[currentStep];
        const state = stages[stageName];

        fetch('/api/light', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(state)
        });

        updateUi(state);

        currentStep = (currentStep + 1) % testSequence.length;
    }, 2000);
};

// --- Initialization ---
const init = () => {
    setLang(lang);

    if (window.location.protocol.startsWith('http')) {
        fetchLightState();
        fetchWifiState();
        setInterval(fetchWifiState, 5000);
    } else {
        console.log("Running in local file mode. Skipping initial API calls.");
        updateUi({ r: 128, g: 128, b: 128, intensity: 50 });
        updateWifiUi({ wifi: false, ip: 'N/A' });
    }
};
