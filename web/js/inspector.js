import { api, escapeHtml, showLoading, showToast } from "./api.js";

const tabs = [
  ["hash", "Hash tables"],
  ["dsu", "Disjoint sets"],
  ["heap", "Binary heap"]
];

function renderHashTable(table, label) {
  const buckets = table.buckets.filter((bucket) => bucket.chain.length);
  return `<section class="hash-table"><div class="hash-title"><h3>${label}</h3><span>${table.bucketCount} buckets</span></div><div class="structure-stats"><div><small>Load factor</small><b>${Number(table.loadFactor).toFixed(3)}</b></div><div><small>Min chain</small><b>${table.minChain}</b></div><div><small>Average chain</small><b>${Number(table.averageChain).toFixed(3)}</b></div><div><small>Max chain</small><b>${table.maxChain}</b></div></div>
    <div class="bucket-list">${buckets.length ? buckets.map((bucket) => `<div class="bucket-row"><span class="bucket-index">${bucket.index}</span><div class="bucket-chain">${bucket.chain.map((entry) => `<span class="bucket-entry">${escapeHtml(entry.name)} <small>#${entry.id}</small></span>`).join('<span class="chain-arrow">→</span>')}</div></div>`).join("") : '<div class="empty-state">No occupied buckets.</div>'}</div></section>`;
}

function renderHash(data) {
  return `<div class="hash-grid">${renderHashTable(data.id, "ID hash table")}${renderHashTable(data.name, "Name hash table")}</div>`;
}

function renderDsu(records) {
  if (!records.length) return '<div class="empty-state">No disjoint-set entries yet.</div>';
  return `<p class="subtle structure-intro">Parent, rank, set size, and root status are read directly from the C Union-Find structure.</p><div class="table-wrap"><table><thead><tr><th>User ID</th><th>Parent ID</th><th>Rank</th><th>Set size</th><th>Root</th></tr></thead><tbody>${records.map((item) => `<tr><td>#${item.id}</td><td>#${item.parent}</td><td>${item.rank}</td><td>${item.size}</td><td><span class="tag ${item.root ? "purple" : ""}">${item.root ? "Root" : "Child"}</span></td></tr>`).join("")}</tbody></table></div>`;
}

function renderHeap(items) {
  if (!items.length) return '<div class="empty-state">The C heap is empty. Run a suggestion or popularity query to populate it.</div>';
  return `<p class="subtle structure-intro">Array order and scores are returned by the C binary heap.</p><div class="heap-array">${items.map((item, index) => `<div class="heap-array-item"><span>${index}</span><strong>${item.score}</strong><small>user #${item.id}</small></div>`).join("")}</div>`;
}

export async function renderInspector(container) {
  container.innerHTML = `<div class="page-heading"><div><p class="eyebrow">Live internals</p><h1>Data structure inspector</h1><p class="page-subtitle">Inspect the current C hash tables, Union-Find state, and binary heap.</p></div></div>
    <section class="panel"><div class="inspector-tabs" role="tablist" aria-label="Data structures">${tabs.map(([id, label], index) => `<button class="inspector-tab${index === 0 ? " active" : ""}" type="button" role="tab" aria-selected="${index === 0}" data-structure="${id}">${label}</button>`).join("")}</div><div class="panel-body" id="inspector-content" role="tabpanel"><div class="loading"><span class="spinner"></span>Loading from the C engine…</div></div></section>`;
  const output = container.querySelector("#inspector-content");
  const loaders = {
    hash: async () => renderHash(await api("/structures/hash")),
    dsu: async () => renderDsu(await api("/structures/dsu")),
    heap: async () => renderHeap(await api("/structures/heap"))
  };
  const load = async (name) => {
    showLoading(output);
    try {
      output.innerHTML = await loaders[name]();
    } catch (error) {
      output.innerHTML = `<div class="empty-state">${escapeHtml(error.message)}</div>`;
      showToast(error.message, true);
    }
  };
  container.querySelectorAll("[data-structure]").forEach((button) => {
    button.addEventListener("click", () => {
      container.querySelectorAll("[data-structure]").forEach((tab) => {
        const active = tab === button;
        tab.classList.toggle("active", active);
        tab.setAttribute("aria-selected", String(active));
      });
      load(button.dataset.structure);
    });
  });
  await load("hash");
}
