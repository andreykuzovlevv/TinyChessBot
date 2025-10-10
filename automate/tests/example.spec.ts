import { test, expect } from '@playwright/test';
import { chromium } from 'playwright';

test('attach and show tab', async () => {
  const browser = await chromium.connectOverCDP('http://127.0.0.1:9222');

  // Use the persistent context that maps to the Chrome profile you started.
  const [context] = browser.contexts();

  // Open a tab and FORCE it to the foreground.
  const page = await context.newPage();
  await page.bringToFront();

  await page.goto('https://chess.com/variants/tinyhouse', { waitUntil: 'networkidle' });

  await page.getByRole('button', { name: 'Δ Analysis' }).click();

  await page.locator('.piece').nth(3).click();



});
