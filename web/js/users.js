import { api, escapeHtml, initials, showLoading, showToast } from "./api.js";

let currentSort = "name";
let currentSearch = "";

function userTable(users) {
  const filtered = users.filter((user) => {
    const query = currentSearch.toLowerCase();
    return !query || String(user.id).includes(query) ||
      user.name.toLowerCase().includes(query) || user.city.toLowerCase().includes(query);
  });
  if (!filtered.length) {
    return '<div class="empty-state"><div class="empty-icon">♙</div><strong>No users found</strong><p>Try a different search or add a user to the directory.</p></div>';
  }
  return `<div class="table-wrap"><table>
    <thead><tr><th>User</th><th>Directory ID</th><th>Age</th><th>City</th><th>Friends</th><th>Actions</th></tr></thead>
    <tbody>${filtered.map((user) => `<tr>
      <td><div class="user-cell"><div class="user-avatar">${escapeHtml(initials(user.name))}</div><div><div class="user-name">${escapeHtml(user.name)}</div><div class="subtle">ConnectSphere member</div></div></div></td>
      <td><span class="tag">#${user.id}</span></td><td>${user.age}</td><td>${escapeHtml(user.city)}</td>
      <td><span class="tag purple">${user.friendCount} connections</span></td>
      <td><div class="row-actions"><button class="text-button" data-action="profile" data-id="${user.id}">Profile</button><button class="text-button" data-action="edit" data-id="${user.id}">Edit</button><button class="text-button" data-action="delete" data-id="${user.id}">Delete</button></div></td>
    </tr>`).join("")}</tbody></table></div>`;
}

export async function renderUsers(container) {
  currentSearch = "";
  container.innerHTML = `
    <div class="page-heading"><div><p class="eyebrow">People directory</p><h1>Users</h1><p class="page-subtitle">Manage the people in your network and explore their connections.</p></div><button class="button" data-action="add">＋ Add user</button></div>
    <section class="panel">
      <div class="panel-header"><h2>User directory</h2><span class="tag" id="user-count">Loading…</span></div>
      <div class="panel-body"><div class="toolbar">
        <div class="toolbar-group"><label class="search-box"><span>⌕</span><input id="user-search" type="search" placeholder="Search name, ID, or city"></label></div>
        <div class="toolbar-group"><label class="subtle" for="user-sort">Sort by</label><select id="user-sort"><option value="name">Name · AVL tree</option><option value="age">Age · Merge sort</option><option value="friends">Friend count · Merge sort</option></select></div>
      </div></div>
      <div id="users-table"><div class="loading"><span class="spinner"></span>Loading users…</div></div>
    </section>
    <div id="users-pagination" class="subtle" style="padding:14px 2px"></div>`;
  const load = async () => {
    const target = container.querySelector("#users-table");
    showLoading(target);
    try {
      const users = await api(`/users?sort=${currentSort}`);
      container.querySelector("#user-count").textContent = `${users.length} total`;
      target.innerHTML = userTable(users);
      container.querySelector("#users-pagination").textContent =
        `Showing ${users.length} ${users.length === 1 ? "person" : "people"} · ordered by ${currentSort}`;
    } catch (error) {
      target.innerHTML = `<div class="empty-state">${escapeHtml(error.message)}</div>`;
      showToast(error.message, true);
    }
  };
  await load();
  container.querySelector("#user-sort").value = currentSort;
  container.querySelector("#user-sort").addEventListener("change", async (event) => {
    currentSort = event.target.value;
    await load();
  });
  container.querySelector("#user-search").addEventListener("input", async (event) => {
    currentSearch = event.target.value;
    await load();
    const input = container.querySelector("#user-search");
    input.focus();
    input.setSelectionRange(currentSearch.length, currentSearch.length);
  });
  container.addEventListener("click", async (event) => {
    const button = event.target.closest("[data-action]");
    if (!button) return;
    const action = button.dataset.action;
    if (action === "add") return openUserModal();
    const id = Number(button.dataset.id);
    if (action === "profile") return openProfile(id);
    const user = await api(`/users/${id}`);
    if (action === "edit") openUserModal(user);
    if (action === "delete") {
      if (!confirm(`Delete ${user.name} and all their friendships?`)) return;
      try {
        await api(`/users/${id}`, { method: "DELETE" });
        showToast(`${user.name} was removed from the directory.`);
        await load();
      } catch (error) { showToast(error.message, true); }
    }
  });
  async function openUserModal(user = null) {
    const root = document.querySelector("#modal-root");
    root.innerHTML = `<div class="modal-backdrop"><section class="modal" role="dialog" aria-modal="true" aria-labelledby="user-modal-title">
      <div class="drawer-head"><div><p class="eyebrow">${user ? "Update directory record" : "New directory record"}</p><h2 id="user-modal-title">${user ? "Edit user" : "Add a user"}</h2></div><button type="button" data-close aria-label="Close dialog">×</button></div>
      <p>Changes are applied to the hash tables, AVL directory, graph, and friend groups in the C engine.</p>
      <form id="user-form"><div class="form-grid">
        ${user ? "" : '<div class="form-field"><label for="user-id">User ID</label><input id="user-id" name="id" type="number" min="1" max="1000000" required></div>'}
        <div class="form-field"><label for="user-name">Full name</label><input id="user-name" name="name" maxlength="49" pattern="[A-Za-z][A-Za-z .'\-]{0,48}" required value="${escapeHtml(user?.name || "")}"></div>
        <div class="form-field"><label for="user-age">Age</label><input id="user-age" name="age" type="number" min="1" max="120" required value="${user?.age ?? ""}"></div>
        <div class="form-field"><label for="user-city">City</label><input id="user-city" name="city" maxlength="39" required value="${escapeHtml(user?.city || "")}"></div>
      </div><div class="form-actions"><button class="button secondary" type="button" data-close>Cancel</button><button class="button" type="submit">${user ? "Save changes" : "Create user"}</button></div></form>
    </section></div>`;
    root.querySelector("[data-close]").addEventListener("click", () => { root.innerHTML = ""; });
    root.querySelectorAll(".modal-backdrop").forEach((backdrop) =>
      backdrop.addEventListener("click", (event) => { if (event.target === backdrop) root.innerHTML = ""; }));
    root.querySelectorAll("[data-close]").forEach((button) =>
      button.addEventListener("click", () => { root.innerHTML = ""; }));
    root.querySelector("#user-form").addEventListener("submit", async (event) => {
      event.preventDefault();
      const form = new FormData(event.currentTarget);
      const body = {
        name: form.get("name").trim(), age: Number(form.get("age")),
        city: form.get("city").trim()
      };
      if (!user) body.id = Number(form.get("id"));
      try {
        await api(user ? `/users/${user.id}` : "/users", {
          method: user ? "PUT" : "POST", body: JSON.stringify(body)
        });
        root.innerHTML = "";
        showToast(user ? "User details updated." : "User added to the network.");
        await load();
      } catch (error) { showToast(error.message, true); }
    });
  }

  async function openProfile(id) {
    const root = document.querySelector("#drawer-root");
    root.innerHTML = '<div class="drawer-backdrop"><aside class="drawer"><div class="loading"><span class="spinner"></span>Loading profile…</div></aside></div>';
    try {
      const [user, friends, group] = await Promise.all([
        api(`/users/${id}`), api(`/users/${id}/friends`), api(`/users/${id}/group`)
      ]);
      root.innerHTML = `<div class="drawer-backdrop"><aside class="drawer" role="dialog" aria-modal="true" aria-label="User profile">
        <div class="drawer-head"><div><p class="eyebrow">Network profile</p><span class="tag purple">Member #${user.id}</span></div><button data-close aria-label="Close profile">×</button></div>
        <div class="profile-head"><div class="user-avatar">${escapeHtml(initials(user.name))}</div><div><h2>${escapeHtml(user.name)}</h2><span class="subtle">${escapeHtml(user.city)} · age ${user.age}</span></div></div>
        <div class="detail-list"><div class="detail-row"><span>Direct friends</span><span>${user.friendCount}</span></div><div class="detail-row"><span>Friend group</span><span>${group.size} members · root ${group.rootId}</span></div></div>
        <div class="panel-header" style="padding-left:0;padding-right:0;margin-top:22px"><h3>Friends</h3><span class="tag">${friends.length}</span></div>
        ${friends.length ? friends.map((friend) => `<div class="detail-row"><div class="user-cell"><div class="user-avatar">${escapeHtml(initials(friend.name))}</div><span class="user-name">${escapeHtml(friend.name)}</span></div><span class="subtle">#${friend.id}</span></div>`).join("") : '<div class="empty-state">No direct friends yet.</div>'}
      </aside></div>`;
      root.querySelector("[data-close]").addEventListener("click", () => { root.innerHTML = ""; });
      root.querySelector(".drawer-backdrop").addEventListener("click", (event) => {
        if (event.target.classList.contains("drawer-backdrop")) root.innerHTML = "";
      });
    } catch (error) {
      root.innerHTML = "";
      showToast(error.message, true);
    }
  }
}
