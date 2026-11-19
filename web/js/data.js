import { api, escapeHtml, showLoading, showToast } from "./api.js";

export async function renderDataManagement(container) {
  container.innerHTML = `<div class="page-heading"><div><p class="eyebrow">Workspace controls</p><h1>Data management</h1><p class="page-subtitle">Review in-memory network data and manage the server's data files.</p></div><button class="button secondary" id="data-refresh">↻ Refresh status</button></div>
    <section class="panel"><div class="panel-header"><div><h2>Current workspace</h2><p class="subtle" style="margin:5px 0 0">These counts reflect the live C engine, not necessarily the saved files.</p></div><span class="tag purple">In memory</span></div><div class="data-stats" id="data-stats"><div class="loading"><span class="spinner"></span>Reading workspace…</div></div></section>
    <section class="panel data-actions-panel"><div class="panel-header"><div><h2>Data operations</h2><p class="subtle" style="margin:5px 0 0">Review each action carefully; several replace the current in-memory network.</p></div></div><div class="data-actions">
      <article class="data-action-card"><span class="data-action-icon violet">✳</span><h3>Load sample network</h3><p>Replace the live workspace with the built-in demo users and friendships. This does not save to disk.</p><button class="button secondary" data-action="sample">Load sample</button></article>
      <article class="data-action-card"><span class="data-action-icon blue">↧</span><h3>Load saved files</h3><p>Replace the live workspace with <code>data/users.txt</code> and <code>data/friendships.txt</code> from the server working directory.</p><button class="button secondary" data-action="load">Load from disk</button></article>
      <article class="data-action-card"><span class="data-action-icon mint">↥</span><h3>Save current workspace</h3><p>Write the live users and friendships to the server's <code>data/</code> files, replacing their current contents.</p><button class="button" data-action="save">Save to disk</button></article>
      <article class="data-action-card"><span class="data-action-icon rose">⌫</span><h3>Reset live workspace</h3><p>Clear all users and friendships from memory. Saved files remain unchanged.</p><button class="button danger" data-action="reset">Reset workspace</button></article>
    </div><div class="data-feedback" id="data-feedback" aria-live="polite"></div></section>
    <div class="data-warning"><strong>Persistence note</strong><span>Changes made in Users and Friendships are in memory until saved. Loading sample, loading saved files, or resetting discards unsaved in-memory changes.</span></div>`;

  const stats = container.querySelector("#data-stats");
  const feedback = container.querySelector("#data-feedback");
  const refresh = async () => {
    showLoading(stats);
    try {
      const [users, graph, groups] = await Promise.all([
        api("/users"), api("/graph"), api("/groups/ranked")
      ]);
      if (!container.contains(stats)) return;
      const values = [
        ["Users", users.length],
        ["Friendships", graph.edges.length],
        ["Communities", groups.length]
      ];
      stats.innerHTML = values.map(([label, value]) => `<div><small>${label}</small><strong>${value}</strong></div>`).join("");
    } catch (error) {
      if (!container.contains(stats)) return;
      stats.innerHTML = `<div class="empty-state">${escapeHtml(error.message)}</div>`;
      showToast(error.message, true);
    }
  };
  const confirmations = {
    sample: "Load the sample network and discard unsaved in-memory changes?",
    load: "Load the saved files and discard unsaved in-memory changes?",
    save: "Overwrite the server's current data/users.txt and data/friendships.txt files?",
    reset: "Reset the in-memory workspace? Saved files will not be changed."
  };
  const successMessages = {
    sample: (result) => `Sample network loaded: ${result.users} users and ${result.friendships} friendships.`,
    load: (result) => `Loaded ${result.users} users from saved data.`,
    save: () => "Workspace saved to the server's data files.",
    reset: () => "In-memory workspace reset. Saved files are unchanged."
  };
  const runAction = async (action, button) => {
    if (!window.confirm(confirmations[action])) return;
    const buttons = container.querySelectorAll("[data-action]");
    buttons.forEach((item) => { item.disabled = true; });
    feedback.textContent = `${action === "save" ? "Saving" : "Applying"} operation…`;
    try {
      const result = await api(`/data/${action}`, { method: "POST" });
      if (!container.contains(feedback)) return;
      feedback.textContent = successMessages[action](result);
      showToast(feedback.textContent);
      await refresh();
    } catch (error) {
      if (!container.contains(feedback)) return;
      feedback.textContent = `Operation failed: ${error.message}`;
      showToast(error.message, true);
    } finally {
      buttons.forEach((item) => { item.disabled = false; });
      if (!container.contains(button)) return;
    }
  };
  container.querySelectorAll("[data-action]").forEach((button) =>
    button.addEventListener("click", () => runAction(button.dataset.action, button)));
  container.querySelector("#data-refresh").addEventListener("click", refresh);
  await refresh();
}
