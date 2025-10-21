document.addEventListener('DOMContentLoaded', () => {
    const langSelector = document.querySelector('.lang-selector');
    const networksSelect = document.getElementById('networks');
    const hiddenSsidCheck = document.getElementById('hidden-ssid-check');
    const manualSsidGroup = document.getElementById('manual-ssid-group');
    const manualSsidInput = document.getElementById('manual-ssid');
    const passwordInput = document.getElementById('password');
    const connectBtn = document.getElementById('connect-btn');
    const refreshBtn = document.getElementById('refresh-btn');
    const feedbackDiv = document.getElementById('feedback');

    let translations = {};

    async function setLanguage(lang) {
        await fetch(`/api/lang`, {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({ lang: lang })
        });

        const response = await fetch(`/locales/${lang}.json`);
        translations = await response.json();

        document.querySelectorAll('[data-i18n]').forEach(el => {
            const key = el.getAttribute('data-i18n');
            if (translations[key]) {
                el.textContent = translations[key];
                if (el.tagName === 'INPUT' || el.tagName === 'BUTTON') {
                    el.value = translations[key];
                }
            }
        });
        document.documentElement.lang = lang;
    }

    async function fetchNetworks(force = false) {
        try {
            const response = await fetch(`/api/wifi/scan?force=${force}`);
            const data = await response.json();

            networksSelect.innerHTML = ''; // Clear existing options
            if (data.networks && data.networks.length > 0) {
                data.networks.forEach(net => {
                    const option = document.createElement('option');
                    option.value = net.ssid;
                    option.textContent = `${net.ssid} (${net.rssi} dBm, Ch: ${net.channel}) ${net.secure ? '🔒' : ''}`;
                    networksSelect.appendChild(option);
                });
            } else {
                const option = document.createElement('option');
                option.textContent = 'No networks found';
                networksSelect.appendChild(option);
            }
        } catch (error) {
            console.error('Error fetching networks:', error);
        }
    }

    function showFeedback(type, messageKey, extra = '') {
        feedbackDiv.className = `feedback-${type}`;
        const message = translations[messageKey] || messageKey;
        feedbackDiv.textContent = `${message} ${extra}`;
        feedbackDiv.classList.remove('hidden');
    }

    hiddenSsidCheck.addEventListener('change', () => {
        if (hiddenSsidCheck.checked) {
            manualSsidGroup.classList.remove('hidden');
            networksSelect.disabled = true;
        } else {
            manualSsidGroup.classList.add('hidden');
            networksSelect.disabled = false;
        }
    });

    refreshBtn.addEventListener('click', () => fetchNetworks(true));

    connectBtn.addEventListener('click', async () => {
        const ssid = hiddenSsidCheck.checked ? manualSsidInput.value : networksSelect.value;
        const password = passwordInput.value;

        if (!ssid) {
            showFeedback('error', 'error.INVALID_INPUT', '(SSID cannot be empty)');
            return;
        }

        showFeedback('info', 'wifi.connecting');

        try {
            const response = await fetch('/api/wifi/connect', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({ ssid, password })
            });
            const data = await response.json();

            if (data.success) {
                showFeedback('success', 'wifi.connected', data.ip);
                setTimeout(() => { window.location.href = '/'; }, 3000);
            } else {
                const errorMsg = translations[`error.${data.error_code}`] || data.message;
                showFeedback('error', 'wifi.error', errorMsg);
            }
        } catch (error) {
            showFeedback('error', 'wifi.error', 'Request failed.');
        }
    });

    langSelector.addEventListener('click', (e) => {
        if (e.target.dataset.lang) {
            setLanguage(e.target.dataset.lang);
        }
    });

    async function init() {
        // Determine initial language
        const langRes = await fetch('/api/lang');
        const langData = await langRes.json();
        const initialLang = langData.lang || (navigator.language.startsWith('es') ? 'es' : 'en');
        await setLanguage(initialLang);

        await fetchNetworks();
    }

    init();
});
