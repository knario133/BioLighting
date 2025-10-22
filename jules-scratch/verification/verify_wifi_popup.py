import os
from playwright.sync_api import sync_playwright, expect

def run():
    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        page = browser.new_page()

        # Obtener la ruta absoluta del archivo HTML
        html_file_path = os.path.abspath('data/ui_web/index.html')

        # Navegar al archivo local
        page.goto(f'file://{html_file_path}')

        # Hacer clic en el botón para abrir el popup
        change_wifi_button = page.locator('#btnChangeWifi')
        expect(change_wifi_button).to_be_visible()
        change_wifi_button.click()

        # Esperar a que el popup de SweetAlert2 esté visible
        swal_popup = page.locator('.swal2-popup')
        expect(swal_popup).to_be_visible()

        # Esperar a que el texto de ERROR (esperado en un entorno de archivo) esté visible
        # Usamos la clave de traducción en español como referencia
        expect(swal_popup).to_contain_text('No se pudo conectar a la red seleccionada.')

        # Tomar captura de pantalla
        page.screenshot(path='jules-scratch/verification/verification.png')

        browser.close()

if __name__ == "__main__":
    run()
