import { api, escapeHtml, showToast, userOptions } from "./api.js";

export async function renderFriendships(container) {
  container.innerHTML = `
    <div class="page-heading"><div><p class="eyebrow">Undirected graph edges</p><h1>Friendships</h1><p class="page-subtitle">Manage direct connections and check component connectivity.</p></div></div>
    <div class="friendship-grid">
      <section class="panel friendship-panel"><div class="panel-header"><h2>Manage a friendship</h2><span class="tag purple">Adjacency list</span></div>
        <div class="panel-body"><form id="friendship-form">
          <div class="form-field"><label for="edge-first">First person</label><select id="edge-first" required><option value="">Choose a user</option></select></div>
          <div class="form-field" style="margin-top:13px"><label for="edge-second">Second person</label><select id="edge-second" required><option value="">Choose a user</option></select></div>
          <div class="form-actions"><button class="button secondary" type="button" id="remove-edge">Remove edge</button><button class="button" type="submit">＋ Add connection</button></div>
        </form><p class="subtle" style="line-height:1.6;margin:13px 0 0">Friendships are undirected. Removing an edge rebuilds the disjoint set from the remaining graph.</p></div>
      </section>
      <section class="panel friendship-panel"><div class="panel-header"><h2>Connectivity check</h2><span class="tag">Union-Find</span></div>
        <div class="panel-body"><form id="connectivity-form">
          <div class="form-grid"><div class="form-field"><label for="check-first">Person A</label><select id="check-first" required><option value="">Choose a user</option></select></div><div class="form-field"><label for="check-second">Person B</label><select id="check-second" required><option value="">Choose a user</option></select></div></div>
          <div class="form-actions"><button class="button" type="submit">Check connection</button></div>
        </form><div id="connection-result"></div></div>
      </section>
    </div>
    <section class="panel" style="margin-top:18px"><div class="panel-header"><div><h2>How connectivity works</h2><p class="subtle" style="margin:5px 0 0">A fast component query from the C disjoint-set structure.</p></div><span class="tag purple">Path compression</span></div><div class="panel-body subtle" style="line-height:1.8">Two users can be connected through a chain of friendships even when they are not direct friends. The result includes each user's DSU root and the traced lookup path so you can see the structure at work.</div></section>`;
  try {
    const users = await api("/users");
    const options = userOptions(users);
    for (const id of ["edge-first", "edge-second", "check-first", "check-second"])
      container.querySelector(`#${id}`).insertAdjacentHTML("beforeend", options);
  } catch (error) {
    showToast(error.message, true);
  }
  const form = container.querySelector("#friendship-form");
  const firstSelect = container.querySelector("#edge-first");
  const secondSelect = container.querySelector("#edge-second");
  const submitEdge = async (method) => {
    if (!form.reportValidity()) return;
    const first = Number(firstSelect.value), second = Number(secondSelect.value);
    try {
      await api("/friendships", {
        method,
        body: JSON.stringify({ id1: first, id2: second })
      });
      showToast(method === "POST" ? "Friendship added." : "Friendship removed.");
    } catch (error) { showToast(error.message, true); }
  };
  form.addEventListener("submit", (event) => {
    event.preventDefault();
    submitEdge("POST");
  });
  container.querySelector("#remove-edge").addEventListener("click", () => submitEdge("DELETE"));
  container.querySelector("#connectivity-form").addEventListener("submit", async (event) => {
    event.preventDefault();
    const first = Number(container.querySelector("#check-first").value);
    const second = Number(container.querySelector("#check-second").value);
    try {
      const result = await api(`/connected?u=${first}&v=${second}`);
      const output = container.querySelector("#connection-result");
      output.className = `connection-result ${result.connected ? "connected" : "disconnected"}`;
      output.innerHTML = `<strong>${result.connected ? "Connected in the same community" : "Not connected"}</strong>
        <p>DSU roots: <b>${escapeHtml(first)}</b> → <span class="connection-path">${result.rootU}</span>;
        <b>${escapeHtml(second)}</b> → <span class="connection-path">${result.rootV}</span><br>
        Find path for first user: <span class="connection-path">${result.pathU.join(" → ")}</span></p>`;
    } catch (error) { showToast(error.message, true); }
  });
}
