<script setup lang="ts">
import { ref } from 'vue'
import AnimeJs from './components/AnimeJs.vue'
import TypewriterCursor from './components/TypewriterCursor.vue'

const lines = ['第一行', '第二行更长一点', 'Hello, Vue 3']
const currentIndex = ref(0)

const animeDemoParams = {
  x: [0, 120, 0],
  rotate: { from: '-8deg' },
  duration: 1400,
  ease: 'inOutQuad' as const,
  loop: true,
  alternate: true,
}

function nextLine() {
  currentIndex.value = (currentIndex.value + 1) % lines.length
}
</script>

<template>
  <main>
    <h1>TypewriterCursor</h1>
    <p>
      <TypewriterCursor
        :texts="lines"
        :index="currentIndex"
        :cursor-blink-interval-ms="530"
        :char-interval-ms="70"
      />
    </p>
    <p>
      <button type="button" @click="nextLine">下一句（index {{ currentIndex }}）</button>
    </p>

    <section class="anime-demo">
      <h2>AnimeJs</h2>
      <AnimeJs :params="animeDemoParams" tag="span" class="anime-demo__box">
        anime.js 示例
      </AnimeJs>
    </section>
    <p>
      Visit <a href="https://vuejs.org/" target="_blank" rel="noopener">vuejs.org</a> to read the
      documentation
    </p>
  </main>
</template>

<style scoped>
main {
  padding: 1rem;
}

.anime-demo {
  margin-top: 1.5rem;
}

.anime-demo__box {
  display: inline-block;
  padding: 0.5rem 0.75rem;
  border-radius: 0.5rem;
  background: #1a1a2e;
  color: #eee;
}
</style>
