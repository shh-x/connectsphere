import { api, escapeHtml, showLoading, showToast } from "./api.js";

export async function renderPerformance(container) {
  container.innerHTML = `<div class="page-heading"><div><p class="eyebrow">Algorithm benchmarks</p><h1>Performance lab</h1><p class="page-subtitle">Run controlled synthetic benchmarks inside the C engine.</p></div><span class="tag purple">C benchmark suite</span></div>
    <section class="panel"><div class="panel-header"><div><h2>Choose benchmark size</h2><p class="subtle" style="margin:5px 0 0">The report identifies capped datasets and query counts where applicable.</p></div></div>
      <div class="panel-body"><form id="performance-form" class="performance-controls"><div class="form-field"><label for="performance-size">Input size (N)</label><select id="performance-size"><option value="1000">1,000 · quick</option><option value="10000">10,000 · standard</option><option value="50000">50,000 · large</option></select></div><button class="button" type="submit">Run benchmarks →</button></form>
      <div class="benchmark-note"><strong>What is measured?</strong><span>Hash, AVL and linear lookup; merge, bubble and qsort sorting; optimized and naive Union-Find; AVL height.</span></div>
      <div id="performance-results"><div class="empty-state">Choose a dataset size and run the C benchmark suite.</div></div></div></section>`;
  const form = container.querySelector("#performance-form");
  const output = container.querySelector("#performance-results");
  form.addEventListener("submit", async (event) => {
    event.preventDefault();
    const n = Number(container.querySelector("#performance-size").value);
    showLoading(output);
    try {
      const result = await api("/performance/run", {
        method: "POST",
        body: JSON.stringify({ n })
      });
      if (!container.querySelector("#performance-results")) return;
      output.innerHTML = `<pre class="benchmark-report">${escapeHtml(result.report)}</pre>`;
    } catch (error) {
      output.innerHTML = `<div class="empty-state">${escapeHtml(error.message)}</div>`;
      showToast(error.message, true);
    }
  });
}
