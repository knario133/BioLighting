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
            homeChangeWifi: "Cambiar WiFi"
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
            homeChangeWifi: "Change WiFi"
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
            changeWifi: document.getElementById('btn-change-wifi'),
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

        // Update stage buttons
        dom.buttons.off.textContent = t.btnOff;
        dom.buttons.growth.textContent = t.btnGrowth;
        dom.buttons.flowering.textContent = t.btnFlowering;
        dom.buttons.fullSpectrum.textContent = t.btnFullSpectrum;
        dom.buttons.transition.textContent = t.btnTransition;
        dom.buttons.changeWifi.textContent = t.homeChangeWifi;
    }

    // --- API Communication ---
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
        confirmAndSetColor(previewState);
    });

    dom.buttons.changeWifi.addEventListener('click', () => {
        window.location.href = '/setup.html';
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

    dom.resetButton.addEventListener('click', () => {
        confirmAndSetColor({ r: 0, g: 0, b: 0, intensity: 0 });
    });

    dom.buttons.testMode.addEventListener('click', () => {
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
    });

    // --- Test Mode Logic ---
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

            // Fire and forget - send the request but don't wait for it
            fetch('/api/light', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(state)
            });

            // Update the UI locally for immediate feedback
            updateUi(state);

            currentStep = (currentStep + 1) % testSequence.length;
        }, 50); // Send a request every 50ms
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