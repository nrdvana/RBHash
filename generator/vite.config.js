import { fileURLToPath, URL } from 'node:url'
import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

// https://vitejs.dev/config/
export default defineConfig({
  root: 'public',
  plugins: [vue()],
//  resolve: {
//    alias: {
//      '@': fileURLToPath(new URL('./public', import.meta.url))
//    }
//  },
  server: {
    proxy: {
      '/generate': 'http://localhost:3000'
    }
  }
})
