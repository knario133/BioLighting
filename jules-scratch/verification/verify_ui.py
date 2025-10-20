import asyncio
from playwright.async_api import async_playwright
import os

async def main():
    async with async_playwright() as p:
        browser = await p.chromium.launch()
        page = await browser.new_page()

        # Get the absolute path to the HTML file
        html_file_path = os.path.abspath('data/ui_web/index.html')

        # 1. Desktop view
        await page.set_viewport_size({"width": 1280, "height": 800})
        await page.goto(f'file://{html_file_path}')
        await page.screenshot(path="jules-scratch/verification/desktop-view.png")

        # 2. Test Mode
        await page.get_by_role("button", name="Modo de Pruebas").click()
        # Click through the confirmation dialog
        await page.locator('button.swal2-confirm').click()
        await asyncio.sleep(0.5) # Wait for the test mode to start
        await page.screenshot(path="jules-scratch/verification/test-mode.png")
        await page.get_by_role("button", name="Detener Pruebas").click()


        # 3. Mobile view
        await page.set_viewport_size({"width": 375, "height": 667})
        await page.screenshot(path="jules-scratch/verification/mobile-view.png")

        await browser.close()

if __name__ == '__main__':
    asyncio.run(main())
