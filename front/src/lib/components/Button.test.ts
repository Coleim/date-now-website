import { describe, it, expect } from 'vitest';
import { render, screen } from '@testing-library/svelte';
import { createRawSnippet } from 'svelte';
import Button from './Button.svelte';

// Create a test snippet
const textSnippet = (text: string) =>
	createRawSnippet(() => ({
		render: () => `<span>${text}</span>`
	}));

describe('Button', () => {
	it('renders with default props', () => {
		render(Button, {
			props: {
				children: textSnippet('Click me')
			}
		});

		expect(screen.getByRole('button')).toBeInTheDocument();
		expect(screen.getByText('Click me')).toBeInTheDocument();
	});

	it('renders as an anchor when tag is "a"', () => {
		render(Button, {
			props: {
				tag: 'a',
				href: '/test',
				children: textSnippet('Link')
			}
		});

		const link = screen.getByRole('link');
		expect(link).toBeInTheDocument();
		expect(link).toHaveAttribute('href', '/test');
	});

	it('applies variant class', () => {
		render(Button, {
			props: {
				variant: 'primary',
				children: textSnippet('Primary')
			}
		});

		expect(screen.getByRole('button')).toHaveClass('button--primary');
	});

	it('applies custom class', () => {
		render(Button, {
			props: {
				customClass: 'my-custom-class',
				children: textSnippet('Custom')
			}
		});

		expect(screen.getByRole('button')).toHaveClass('my-custom-class');
	});
});
