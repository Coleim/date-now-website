<script lang="ts">
	import ArticlesList from '$lib/components/ArticlesList.svelte';
	import Hero from '$lib/layout/Hero.svelte';
	import type { Article } from '../types';

	const articles: Article[] = [
		{
			title: 'Moltbook, or when the AI bubble gets too big',
			excerpt:
				'Because we never tire of new AI-boosted features, Figma announced yesterday the vectorization of raster images. Now, I don’t know if you’ve ever tried to do this, but every time, it wasn’t great. This feature is available for paid plans and uses AI credits.',
			editionNumber: 72,
			publishedDate: new Date('02/08/2026'),
			content: [],
			tags: [
				{
					name: 'AI',
					color: {
						r: 100,
						g: 75,
						b: 230
					}
				}
			]
		},
		{
			title: 'ChatGPT wants to guess your age',
			subtitle: 'The coming back of Date.now()!',
			excerpt:
				'Welcome back to your favorite newsletter! Thank you for your patience; I needed this break.',
			editionNumber: 69,
			publishedDate: new Date('01/25/2026'),
			content: [],
			tags: [
				{
					name: 'Cybersecurity',
					color: {
						r: 140,
						g: 30,
						b: 20
					}
				},
				{
					name: 'Web',
					color: {
						r: 50,
						g: 230,
						b: 100
					}
				}
			]
		},
		{
			title: 'ChatGPT wants to guess your age',
			subtitle: 'The coming back of Date.now()!',
			excerpt:
				'Welcome back to your favorite newsletter! Thank you for your patience; I needed this break.',
			editionNumber: 69,
			publishedDate: new Date('01/25/2026'),
			content: [],
			tags: [
				{
					name: 'Cybersecurity',
					color: {
						r: 140,
						g: 30,
						b: 20
					}
				},
				{
					name: 'Web',
					color: {
						r: 50,
						g: 230,
						b: 100
					}
				}
			]
		}
	];
</script>

<Hero />

<ArticlesList title="Latest issues" {articles} />
