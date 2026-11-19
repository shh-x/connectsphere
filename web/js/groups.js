import { api, escapeHtml, initials, showLoading, showToast } from "./api.js";

export async function renderGroups(container) {
  container.innerHTML = `<div class="page-heading"><div><p class="eyebrow">Connected components</p><h1>Communities</h1><p class="page-subtitle">Friend groups ranked by size using merge sort in the C engine.</p></div><button class="button secondary" id="groups-refresh">↻ Refresh</button></div>
    <section class="panel"><div class="panel-header"><div><h2>Community ranking</h2><p class="subtle" style="margin:5px 0 0">Largest connected components appear first</p></div><span class="tag purple" id="group-count">Loading…</span></div>
      <div id="community-list" class="community-list"><div class="loading"><span class="spinner"></span>Ranking friend groups…</div></div></section>
    <section class="panel community-note"><div class="panel-header"><h2>How groups are formed</h2><span class="tag">Disjoint sets + merge sort</span></div><div class="panel-body subtle">The graph's connected components are found by the C Union-Find structure. The C merge sort ranks components by descending member count, with root ID as the tie-break.</div></section>`;
  const load = async () => {
    const target = container.querySelector("#community-list");
    showLoading(target);
    try {
      const groups = await api("/groups/ranked");
      const largest = groups.reduce((max, group) => Math.max(max, group.size), 1);
      container.querySelector("#group-count").textContent = `${groups.length} ${groups.length === 1 ? "group" : "groups"}`;
      if (!groups.length) {
        target.innerHTML = '<div class="empty-state"><div class="empty-icon">◎</div><strong>No communities yet</strong><p>Add users and connect them with friendships to create a group.</p></div>';
        return;
      }
      target.innerHTML = groups.map((group, index) => {
        const width = Math.max(4, Math.round(group.size / largest * 100));
        const palette = ["violet", "mint", "amber", "blue", "rose"];
        return `<article class="community-card"><div class="community-rank">#${index + 1}</div><div class="community-main">
          <div class="community-title"><div><h3>${escapeHtml(group.representative)}'s community</h3><span class="subtle">Component root ${group.rootId} · ${group.size} ${group.size === 1 ? "member" : "members"}</span></div><button class="text-button" data-community="${group.rootId}">View members</button></div>
          <div class="community-bar"><span class="${palette[index % palette.length]}" style="width:${width}%"></span></div>
          <div class="community-members" id="members-${group.rootId}">${group.members.map((member) => `<span class="member-chip"><i>${escapeHtml(initials(member.name))}</i>${escapeHtml(member.name)}</span>`).join("")}</div>
        </div></article>`;
      }).join("");
      target.querySelectorAll("[data-community]").forEach((button) => {
        button.addEventListener("click", () => {
          const members = target.querySelector(`#members-${button.dataset.community}`);
          const hidden = members.classList.toggle("collapsed");
          button.textContent = hidden ? "View members" : "Hide members";
        });
      });
    } catch (error) {
      target.innerHTML = `<div class="empty-state">${escapeHtml(error.message)}</div>`;
      showToast(error.message, true);
    }
  };
  await load();
  container.querySelector("#groups-refresh").addEventListener("click", load);
}
