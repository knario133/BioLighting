
import os
from playwright.sync_api import sync_playwright, expect

def run_verification():
    """
    Navigates to the local index.html file and takes a screenshot
    to verify the UI renders correctly.
    """
    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        page = browser.new_page()

        # Construct the absolute file path to index.html
        current_dir = os.getcwd()
        file_path = os.path.join(current_dir, 'data', 'ui_web', 'index.html')
        url = f'file://{file_path}'

        print(f"Navigating to: {url}")
        page.goto(url)

        # Wait for the main landmark to be visible to ensure the page has loaded.
        main_content = page.get_by_role("main")
        expect(main_content).to_be_visible(timeout=5000)

        # Take a screenshot for visual confirmation
        screenshot_path = "jules-scratch/verification/verification.png"
        page.screenshot(path=screenshot_path)
        print(f"Screenshot saved to {screenshot_path}")

        browser.close()

if __name__ == "__main__":
    run_verification()
