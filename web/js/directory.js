import { api, escapeHtml, initials, showLoading, showToast } from "./api.js";

function renderUsers(users) {
  if (!users.length) return '<div class="empty-state">No users match this directory query.</div>';
  return `<div class="table-wrap"><table><thead><tr><th>User</th><th>ID</th><th>Age</th><th>City</th><th>Friends</th></tr></thead><tbody>${users.map((user) => `<tr><td><span class="user-cell"><span class="user-avatar">${escapeHtml(initials(user.name))}</span><span class="user-name">${escapeHtml(user.name)}</span></span></td><td>#${user.id}</td><td>${user.age}</td><td>${escapeHtml(user.city)}</td><td>${user.friendCount}</td></tr>`).join("")}</tbody></table></div>`;
}

function renderTreeNode(node) {
  if (!node) return "";
  const children = [node.left, node.right].some(Boolean);
  return `<li><details class="avl-node" ${children ? "open" : ""}><summary><span class="avl-name">${escapeHtml(node.name)}</span><span class="avl-id">#${node.id}</span><span class="tag">H ${node.height}</span><span class="tag ${Math.abs(node.balance) > 1 ? "danger-tag" : "purple"}">BF ${node.balance}</span></summary>${children ? `<ul>${node.left ? renderTreeNode(node.left) : '<li class="avl-null">∅</li>'}${node.right ? renderTreeNode(node.right) : '<li class="avl-null">∅</li>'}</ul>` : ""}</details></li>`;
}

export async function renderDirectory(container) {
  container.innerHTML = `<div class="page-heading"><div><p class="eyebrow">Self-balancing search tree</p><h1>AVL directory</h1><p class="page-subtitle">Browse names in AVL order, query ranges and prefixes, and inspect node heights.</p></div><button class="button secondary" id="directory-refresh">↻ All users</button></div>
    <section class="panel"><div class="panel-header"><div><h2>Directory search</h2><p class="subtle" style="margin:5px 0 0">Queries execute against the C AVL index.</p></div><span class="tag purple" id="directory-count">Loading…</span></div>
      <div class="panel-body"><div class="directory-forms"><form id="prefix-form" class="directory-form"><div class="form-field"><label for="directory-prefix">Name prefix</label><input id="directory-prefix" name="q" maxlength="127" placeholder="e.g. Ma" required></div><button class="button" type="submit">Search prefix</button></form>
      <form id="range-form" class="directory-form"><div class="form-field"><label for="range-from">From name</label><input id="range-from" name="from" maxlength="127" placeholder="e.g. Ada" required></div><div class="form-field"><label for="range-to">To name</label><input id="range-to" name="to" maxlength="127" placeholder="e.g. Grace" required></div><button class="button secondary" type="submit">Search range</button></form></div>
      <div id="directory-results"><div class="loading"><span class="spinner"></span>Loading alphabetical directory…</div></div></div></section>
    <section class="panel avl-panel"><div class="panel-header"><div><h2>AVL tree</h2><p class="subtle" style="margin:5px 0 0">Tree topology, height, and balance factor returned by the C engine.</p></div><span class="tag purple">AVL</span></div><div class="panel-body" id="avl-tree"><div class="loading"><span class="spinner"></span>Loading tree…</div></div></section>`;

  const result = container.querySelector("#directory-results");
  const count = container.querySelector("#directory-count");
  const loadUsers = async (path) => {
    showLoading(result);
    try {
      const users = await api(path);
      count.textContent = `${users.length} ${users.length === 1 ? "user" : "users"}`;
      result.innerHTML = renderUsers(users);
    } catch (error) {
      result.innerHTML = `<div class="empty-state">${escapeHtml(error.message)}</div>`;
      showToast(error.message, true);
    }
  };

  container.querySelector("#prefix-form").addEventListener("submit", (event) => {
    event.preventDefault();
    const query = container.querySelector("#directory-prefix").value.trim();
    if (query) loadUsers(`/directory/prefix?q=${encodeURIComponent(query)}`);
  });
  container.querySelector("#range-form").addEventListener("submit", (event) => {
    event.preventDefault();
    const from = container.querySelector("#range-from").value.trim();
    const to = container.querySelector("#range-to").value.trim();
    if (from && to) loadUsers(`/directory/range?from=${encodeURIComponent(from)}&to=${encodeURIComponent(to)}`);
  });
  container.querySelector("#directory-refresh").addEventListener("click", () => loadUsers("/users"));
  await Promise.all([
    loadUsers("/users"),
    api("/directory/tree").then((tree) => {
      const target = container.querySelector("#avl-tree");
      if (!target) return;
      target.innerHTML = tree ? `<div class="avl-tree-scroll"><ul class="avl-tree">${renderTreeNode(tree)}</ul></div>` : '<div class="empty-state">The AVL directory is empty.</div>';
    }).catch((error) => {
      const target = container.querySelector("#avl-tree");
      if (!target) return;
      target.innerHTML = `<div class="empty-state">${escapeHtml(error.message)}</div>`;
      showToast(error.message, true);
    })
  ]);
}
