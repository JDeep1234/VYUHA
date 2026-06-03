const canvas = document.querySelector("#signal-canvas");
const ctx = canvas?.getContext("2d", { alpha: true });
const reduceMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;

let width = 0;
let height = 0;
let particles = [];

const palette = ["#42d7ff", "#88f2c2", "#ffd166", "#ff6b8a"];

function resizeCanvas() {
  if (!canvas || !ctx) return;

  const ratio = Math.min(window.devicePixelRatio || 1, 2);
  width = window.innerWidth;
  height = window.innerHeight;
  canvas.width = Math.floor(width * ratio);
  canvas.height = Math.floor(height * ratio);
  canvas.style.width = `${width}px`;
  canvas.style.height = `${height}px`;
  ctx.setTransform(ratio, 0, 0, ratio, 0, 0);

  const count = Math.max(46, Math.min(110, Math.floor((width * height) / 15000)));
  particles = Array.from({ length: count }, (_, index) => ({
    x: Math.random() * width,
    y: Math.random() * height,
    vx: (Math.random() - 0.5) * 0.45,
    vy: (Math.random() - 0.5) * 0.45,
    radius: 0.8 + Math.random() * 1.8,
    color: palette[index % palette.length],
    phase: Math.random() * Math.PI * 2,
  }));
}

function drawNetwork() {
  if (!ctx) return;

  ctx.clearRect(0, 0, width, height);
  const time = performance.now() * 0.001;

  for (const particle of particles) {
    particle.x += particle.vx + Math.cos(time + particle.phase) * 0.08;
    particle.y += particle.vy + Math.sin(time * 1.2 + particle.phase) * 0.08;

    if (particle.x < -20) particle.x = width + 20;
    if (particle.x > width + 20) particle.x = -20;
    if (particle.y < -20) particle.y = height + 20;
    if (particle.y > height + 20) particle.y = -20;
  }

  for (let i = 0; i < particles.length; i += 1) {
    for (let j = i + 1; j < particles.length; j += 1) {
      const a = particles[i];
      const b = particles[j];
      const distance = Math.hypot(a.x - b.x, a.y - b.y);
      if (distance < 126) {
        const alpha = 1 - distance / 126;
        ctx.strokeStyle = `rgba(66, 215, 255, ${alpha * 0.16})`;
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(a.x, a.y);
        ctx.lineTo(b.x, b.y);
        ctx.stroke();
      }
    }
  }

  for (const particle of particles) {
    const glow = 0.58 + Math.sin(time * 2.4 + particle.phase) * 0.28;
    ctx.fillStyle = particle.color;
    ctx.globalAlpha = glow;
    ctx.beginPath();
    ctx.arc(particle.x, particle.y, particle.radius + glow, 0, Math.PI * 2);
    ctx.fill();
  }

  ctx.globalAlpha = 1;
  requestAnimationFrame(drawNetwork);
}

const observer = new IntersectionObserver(
  (entries) => {
    for (const entry of entries) {
      if (entry.isIntersecting) {
        entry.target.classList.add("is-visible");
      }
    }
  },
  { threshold: 0.14 }
);

document.querySelectorAll(".section, .module-card, .visual-grid figure, .artifact-list a").forEach((item) => {
  item.classList.add("reveal");
  observer.observe(item);
});

window.addEventListener("resize", resizeCanvas);
resizeCanvas();

if (!reduceMotion) {
  drawNetwork();
}
