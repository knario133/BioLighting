document.addEventListener('DOMContentLoaded', () => {
    // --- i18n Translations ---
    const translations = {
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
            applyButton: "Aplicar Nuevo Color"
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
            applyButton: "Apply New Color"
        }
    };

    // --- Color & Stage Definitions ---
    const stages = {
        off:          { r: 0,   g: 0,   b: 0,   intensity: 0 },
        growth:       { r: 50,  g: 0,   b: 255, intensity: 100 },
        flowering:    { r: 255, g: 0,   b: 100, intensity: 100 },
        fullSpectrum: { r: 255, g: 255, b: 255, intensity: 100 },
        transition:   { r: 180, g: 0,   b: 180, intensity: 100 }
    };
    const testSequence = ['off', 'growth', 'transition', 'flowering', 'fullSpectrum'];

    // --- DOM Elements ---
    const dom = {
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
        }
    };

    let lang = localStorage.getItem('lang') || document.documentElement.lang || 'es';
    let testModeInterval = null;
    let previewState = { r: 0, g: 0, b: 0, intensity: 100 };

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
    }

    // --- API Communication ---
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

    // --- UI Update & Sync ---
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

    // This function is called ONLY after a successful API call
    // It syncs the preview state and the UI to match the device's actual state.
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

    // Listen to Buttons
    dom.buttons.apply.addEventListener('click', () => {
        setColor(previewState);
    });

    Object.keys(stages).forEach(stageName => {
        if (dom.buttons[stageName]) {
            dom.buttons[stageName].addEventListener('click', () => {
                setColor(stages[stageName]);
            });
        }
    });

    dom.resetButton.addEventListener('click', () => {
        setColor({ r: 0, g: 0, b: 0, intensity: 0 });
    });

    dom.buttons.testMode.addEventListener('click', () => {
        if (testModeInterval) {
            stopTestMode();
        } else {
            startTestMode();
        }
    });

    // --- Test Mode Logic (Largely unchanged, but uses `updateUi`) ---
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
        let lastColor = { r: previewState.r, g: previewState.g, b: previewState.b };

        const runNextFade = () => {
            const targetStageName = testSequence[currentStep];
            const target = stages[targetStageName];

            animateColorFade(lastColor, target, 5000, 50, () => {
                lastColor = { r: target.r, g: target.g, b: target.b };
                currentStep = (currentStep + 1) % testSequence.length;
            });
        };

        testModeInterval = setInterval(runNextFade, 5100);
        runNextFade();
    };

    const animateColorFade = (startRgb, end, duration, interval, onComplete) => {
        const steps = duration / interval;
        const rStep = (end.r - startRgb.r) / steps;
        const gStep = (end.g - startRgb.g) / steps;
        const bStep = (end.b - startRgb.b) / steps;

        let currentR = startRgb.r;
        let currentG = startRgb.g;
        let currentB = startRgb.b;
        let stepCount = 0;

        const fade = setInterval(() => {
            if (!testModeInterval) {
                clearInterval(fade);
                return;
            }

            if (stepCount >= steps) {
                clearInterval(fade);
                // Directly call API, then update UI with response
                fetch('/api/light', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ r: end.r, g: end.g, b: end.b, intensity: end.intensity })
                }).then(res => res.json()).then(updateUi);
                if(onComplete) onComplete();
                return;
            }

            currentR += rStep;
            currentG += gStep;
            currentB += bStep;

            const state = {
                r: Math.round(currentR),
                g: Math.round(currentG),
                b: Math.round(currentB),
                intensity: end.intensity
            };

            // During fade, we also directly call API and update UI
            fetch('/api/light', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(state)
            }).then(res => res.json()).then(updateUi);

            stepCount++;
        }, interval);
    };

    // --- Initialization ---
    const init = () => {
        setLang(lang);
        fetchLightState(); // Fetches initial state and syncs UI
        fetchWifiState();
        setInterval(fetchWifiState, 5000);
    };

    init();
});