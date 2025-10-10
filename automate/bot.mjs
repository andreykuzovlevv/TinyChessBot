import { chromium } from 'playwright';

async function main() {
    const browser = await chromium.connectOverCDP('http://127.0.0.1:9222');

    // Use the persistent context that maps to the Chrome profile you started.
    const [context] = browser.contexts();

    // Open a tab and FORCE it to the foreground.
    const page = await context.newPage();
    await page.bringToFront();

    await page.goto('https://playwright.dev/', { waitUntil: 'domcontentloaded' });
    await expect(page).toHaveTitle(/Playwright/);

    // Keep it visible long enough to see it.
    await page.waitForTimeout(15000);
    // Do not close the context/browser — you’re attached to a real Chrome.
}

main().catch(err => {
    console.error(err);
    process.exit(1);
});
