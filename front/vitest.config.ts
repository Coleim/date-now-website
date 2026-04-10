import { defineConfig } from 'vitest/config';
import { sveltekit } from '@sveltejs/kit/vite';

export default defineConfig({
	plugins: [sveltekit()],
	test: {
		include: ['src/**/*.{test,spec}.{js,ts}'],
		environment: 'jsdom',
		globals: true,
		setupFiles: ['./vitest-setup.ts'],
		alias: {
			// Resolve to browser version of Svelte for client-side testing
			svelte: 'svelte'
		},
		coverage: {
			provider: 'v8',
			include: ['src/**/*.{js,ts,svelte}'],
			exclude: ['src/**/*.test.ts', 'src/**/*.spec.ts', 'src/app.d.ts']
		}
	},
	resolve: {
		conditions: ['browser']
	}
});
