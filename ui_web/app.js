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
        values: {
            r: document.getElementById('r-value'),
            g: document.getElementById('g-value'),
            b: document.getElementById('b-value'),
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
        }
    };

    let lang = localStorage.getItem('lang') || document.documentElement.lang || 'es';
    let debounceTimer;
    let testModeInterval = null;

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
        dom.wifiStatusLabel.textContent = t.wifiStatusLabel;
        dom.ipLabel.textContent = t.ipLabel;
        dom.buttons.testMode.textContent = testModeInterval ? t.stopTestMode : t.testMode;
    }

    // --- API Communication ---
    const setColor = async (state) => {
        stopTestMode(); // Stop test mode on any manual color change
        try {
            const response = await fetch('/api/light', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(state)
            });
            if (response.ok) {
                const newState = await response.json();
                updateUi(newState);
            }
        } catch (error) {
            console.error('Failed to update light state:', error);
        }
    };

    const resetColor = () => {
        setColor({ r: 0, g: 0, b: 0, intensity: 0 });
    };

    const fetchAndUpdateUi = async () => {
        fetchLightState();
        fetchWifiState();
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

    // --- UI Update ---
    const toHex = (c) => `0${(c || 0).toString(16)}`.slice(-2);
    const updateUi = (state) => {
        // Update sliders
        dom.sliders.r.value = state.r;
        dom.values.r.textContent = state.r;
        dom.sliders.g.value = state.g;
        dom.values.g.textContent = state.g;
        dom.sliders.b.value = state.b;
        dom.values.b.textContent = state.b;
        dom.sliders.intensity.value = state.intensity;
        dom.values.intensity.textContent = `${state.intensity}%`;
        // Update color picker
        dom.colorPicker.value = `#${toHex(state.r)}${toHex(state.g)}${toHex(state.b)}`;
    };

    const updateWifiUi = (state) => {
        const t = translations[lang];
        if (state.wifi) {
            dom.wifiStatusValue.textContent = `${t.connected} (${state.ssid})`;
        } else {
            dom.wifiStatusValue.textContent = t.disconnected;
        }
        dom.ipValue.textContent = state.ip;
    };

    // --- Test Mode ---
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
        let lastColor = {
            r: parseInt(dom.sliders.r.value, 10),
            g: parseInt(dom.sliders.g.value, 10),
            b: parseInt(dom.sliders.b.value, 10)
        };

        const runNextFade = () => {
            const targetStageName = testSequence[currentStep];
            const target = stages[targetStageName];

            animateColorFade(lastColor, target, 5000, 50, () => {
                lastColor = { r: target.r, g: target.g, b: target.b };
                currentStep = (currentStep + 1) % testSequence.length;
            });
        };

        testModeInterval = setInterval(runNextFade, 5100); // 5s fade + 100ms buffer
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
            if (!testModeInterval) { // a manual action stopped the test mode
                clearInterval(fade);
                return;
            }

            if (stepCount >= steps) {
                clearInterval(fade);
                setColor({ r: end.r, g: end.g, b: end.b, intensity: end.intensity });
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

            // During fade, we don't use the main setColor function to avoid stopping the loop
            fetch('/api/light', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(state)
            }).then(res => res.json()).then(updateUi);

            stepCount++;
        }, interval);
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

    for (const key in dom.sliders) {
        dom.sliders[key].addEventListener('input', (e) => {
            stopTestMode();
            const value = parseInt(e.target.value, 10);
            if (key === 'intensity') {
                dom.values[key].textContent = `${value}%`;
            } else {
                dom.values[key].textContent = value;
            }

            clearTimeout(debounceTimer);
            debounceTimer = setTimeout(() => {
                const currentState = {
                    r: parseInt(dom.sliders.r.value, 10),
                    g: parseInt(dom.sliders.g.value, 10),
                    b: parseInt(dom.sliders.b.value, 10),
                    intensity: parseInt(dom.sliders.intensity.value, 10)
                };
                setColor(currentState);
            }, 100);
        });
    }

    dom.colorPicker.addEventListener('input', (e) => {
        const rgb = hexToRgb(e.target.value);
        if (rgb) {
            setColor({ ...rgb, intensity: 100 });
        }
    });

    Object.keys(stages).forEach(stageName => {
        if (dom.buttons[stageName]) {
            dom.buttons[stageName].addEventListener('click', () => {
                setColor(stages[stageName]);
            });
        }
    });

    dom.buttons.testMode.addEventListener('click', () => {
        if (testModeInterval) {
            stopTestMode();
        } else {
            startTestMode();
        }
    });

    dom.resetButton.addEventListener('click', resetColor);

    // --- Initialization ---
    const init = () => {
        setLang(lang);
        fetchAndUpdateUi();
        setInterval(fetchWifiState, 5000);
    };

    init();
});