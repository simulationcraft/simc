// @ts-check
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

// https://astro.build/config
export default defineConfig({
	integrations: [
		starlight({
			title: 'SimulationCraft',
			social: [{ icon: 'github', label: 'GitHub', href: 'https://github.com/simulationcraft/simc' }],
			sidebar: [
				{
					label: 'Start Here',
					link: '/'
				},
				{
					label: 'Features',
					link: '/features'
				},
				{
					label: 'Common Issues',
					link: '/common-issues'
				},
				{
					label: 'Frequently Asked Questions',
					link: '/faq'
				},
				{
					label: 'Classes',
					items: [{ autogenerate: { directory: 'classes' }}]
				},
				{
					label: 'Textual Configuration Interface',
					items: [{ autogenerate: { directory: 'reference' }}]
				},
				{
					label: 'Developer Corner',
					items: [{ autogenerate: { directory: 'development' } }]
				},
				{
					label: 'Appendixes',
					items: [{ autogenerate: { directory: 'appendixes' } }]
				}
			],
		}),
	],
});
