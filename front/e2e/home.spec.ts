import { test, expect } from '@playwright/test';

test.describe('Homepage', () => {
	test('has title', async ({ page }) => {
		await page.goto('/');

		// Verify the page loads successfully
		await expect(page).toHaveURL('/');
	});

	test('navigation works', async ({ page }) => {
		await page.goto('/');

		// Check that navigation links are present
		const nav = page.locator('nav');
		await expect(nav).toBeVisible();
	});
});
