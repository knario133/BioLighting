document.addEventListener('DOMContentLoaded', () => {
    // --- i18n Translations ---
    const translations = {
        es: {
            title: "Control de Iluminación",
            labelR: "Rojo:",
            labelG: "Verde:",
            labelB: "Azul:",
            labelIntensity: "Intensidad:",
            presetsTitle: "Presets",
            actionsTitle: "Acciones",
            resetButton: "Resetear",
            confirmTitle: "Confirmar Acción",
            confirmText: "¿Estás seguro de que quieres aplicar este cambio?",
            confirmPresetText: (preset) => `¿Aplicar el preset '${preset}'?`,
            confirmResetText: "¿Restablecer el color a los valores por defecto?",
            yes: "Sí",
            no: "No"
        },
        en: {
            title: "Lighting Control",
            labelR: "Red:",
            labelG: "Green:",
            labelB: "Blue:",
            labelIntensity: "Intensity:",
            presetsTitle: "Presets",
            actionsTitle: "Actions",
            resetButton: "Reset",
            confirmTitle: "Confirm Action",
            confirmText: "Are you sure you want to apply this change?",
            confirmPresetText: (preset) => `Apply the '${preset}' preset?`,
            confirmResetText: "Reset the color to default values?",
            yes: "Yes",
            no: "No"
        }
    };

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
            intensity: document.getElementById('label-intensity')
        },
        presetsTitle: document.getElementById('presets-title'),
        actionsTitle: document.getElementById('actions-title'),
        presetButtons: {
            warm: document.getElementById('preset-warm'),
            cool: document.getElementById('preset-cool'),
            sunset: document.getElementById('preset-sunset')
        },
        resetButton: document.getElementById('reset-button')
    };

    let lang = localStorage.getItem('lang') || document.documentElement.lang || 'es';
    let debounceTimer;

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
        dom.presetsTitle.textContent = t.presetsTitle;
        dom.actionsTitle.textContent = t.actionsTitle;
        dom.resetButton.textContent = t.resetButton;
    }

    dom.langSelect.value = lang;
    dom.langSelect.addEventListener('change', e => setLang(e.target.value));
    setLang(lang);

    // --- SweetAlert2 Helper ---
    function confirmAction(text, callback) {
        const t = translations[lang];
        Swal.fire({
            title: t.confirmTitle,
            text: text,
            icon: 'question',
            showCancelButton: true,
            confirmButtonText: t.yes,
            cancelButtonText: t.no,
            confirmButtonColor: '#3085d6',
            cancelButtonColor: '#6b7280'
        }).then(result => {
            if (result.isConfirmed) {
                callback();
            }
        });
    }

    // --- API Communication ---
    const updateLightState = async (state) => {
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

    const applyPreset = async (presetName) => {
        confirmAction(translations[lang].confirmPresetText(presetName), async () => {
            try {
                const response = await fetch(`/api/preset/${presetName}`, { method: 'POST' });
                if (response.ok) {
                    const newState = await response.json();
                    updateUi(newState);
                }
            } catch (error) {
                console.error(`Failed to apply preset ${presetName}:`, error);
            }
        });
    };

    const resetColor = () => {
        confirmAction(translations[lang].confirmResetText, () => {
             updateLightState({ r: 0, g: 0, b: 0, intensity: 100 });
        });
    };

    const fetchAndUpdateUi = async () => {
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

    // --- UI Update ---
    const updateUi = (state) => {
        dom.sliders.r.value = state.r;
        dom.values.r.textContent = state.r;
        dom.sliders.g.value = state.g;
        dom.values.g.textContent = state.g;
        dom.sliders.b.value = state.b;
        dom.values.b.textContent = state.b;
        dom.sliders.intensity.value = state.intensity;
        dom.values.intensity.textContent = `${state.intensity}%`;
    };

    // --- Event Listeners ---
    for (const key in dom.sliders) {
        dom.sliders[key].addEventListener('input', (e) => {
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
                updateLightState(currentState);
            }, 100);
        });
    }

    for (const presetName in dom.presetButtons) {
        dom.presetButtons[presetName].addEventListener('click', () => {
            applyPreset(presetName);
        });
    }

    dom.resetButton.addEventListener('click', resetColor);

    // --- Initialization ---
    fetchAndUpdateUi();
    setInterval(fetchAndUpdateUi, 2000);
});
