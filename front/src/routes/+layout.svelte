<script lang="ts">
	import favicon from '$lib/assets/favicon.svg';
	import { page } from '$app/state';
	import Header from '$lib/components/Header.svelte';
	import Footer from '$lib/components/Footer.svelte';
	import '../lib/styles/global.scss';

	let { children } = $props();
</script>

<svelte:head>
	<link rel="icon" href={favicon} />
</svelte:head>

<Header currentPath={page.url.pathname} />

<main class="main">
	{@render children()}
</main>

<Footer />

<style>
	.main {
		display: flex;
		flex-direction: column;
		gap: 20px;
		flex: 1;
	}
</style>
