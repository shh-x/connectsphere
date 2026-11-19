import { api, escapeHtml, showLoading, showToast } from "./api.js";

const colors = ["#6258e8", "#24a99a", "#e39b4a", "#557fe0", "#df6269", "#8d62c6"];
const width = 1000;
const height = 620;

function layoutNodes(nodes) {
  const groups = new Map();
  nodes.forEach((node) => {
    if (!groups.has(node.groupId)) groups.set(node.groupId, []);
    groups.get(node.groupId).push(node);
  });
  const entries = Array.from(groups.entries());
  const columns = Math.max(1, Math.ceil(Math.sqrt(entries.length)));
  const rows = Math.max(1, Math.ceil(entries.length / columns));
  const cellWidth = width / columns;
  const cellHeight = height / rows;
  const positions = new Map();
  entries.forEach(([, members], groupIndex) => {
    const centerX = (groupIndex % columns + 0.5) * cellWidth;
    const centerY = (Math.floor(groupIndex / columns) + 0.5) * cellHeight;
    const radius = Math.min(175, cellWidth * 0.37, cellHeight * 0.36);
    members.forEach((node, index) => {
      const angle = -Math.PI / 2 + 2 * Math.PI * index / members.length;
      const distance = members.length === 1 ? 0 : radius;
      positions.set(node.id, {
        x: centerX + Math.cos(angle) * distance,
        y: centerY + Math.sin(angle) * distance,
        color: colors[groupIndex % colors.length]
      });
    });
  });
  return positions;
}

function renderGraph(graph, selectedId = "") {
  const positions = layoutNodes(graph.nodes);
  const edges = graph.edges.map((edge) => {
    const source = positions.get(edge.source);
    const target = positions.get(edge.target);
    return source && target ? `<line class="network-edge" data-source="${edge.source}" data-target="${edge.target}" x1="${source.x}" y1="${source.y}" x2="${target.x}" y2="${target.y}"></line>` : "";
  }).join("");
  const nodes = graph.nodes.map((node) => {
    const point = positions.get(node.id);
    const chosen = String(node.id) === selectedId;
    const label = escapeHtml(node.name);
    return `<g class="network-node${chosen ? " selected" : ""}" data-node="${node.id}" tabindex="0" role="button" aria-label="${label}, ${node.friendCount} friends">
      <circle cx="${point.x}" cy="${point.y}" r="${chosen ? 17 : 13}" fill="${point.color}"></circle>
      <text x="${point.x}" y="${point.y + 27}" text-anchor="middle">${label}</text>
      <title>${label} · ${node.friendCount} friends · community ${node.groupId}</title></g>`;
  }).join("");
  return `<svg class="network-svg" viewBox="0 0 ${width} ${height}" role="img" aria-label="Friendship network graph">${edges}${nodes}</svg>`;
}

export async function renderNetwork(container) {
  container.innerHTML = `<div class="page-heading"><div><p class="eyebrow">Social graph visualization</p><h1>Network graph</h1><p class="page-subtitle">Friendship edges and community IDs come from the C graph and Union-Find structures.</p></div><button class="button secondary" id="network-refresh">↻ Refresh graph</button></div>
    <section class="panel"><div class="panel-header"><div><h2>Friendship network</h2><p class="subtle" style="margin:5px 0 0">Choose or select a person to highlight their connections.</p></div><span class="tag purple" id="network-count">Loading…</span></div>
      <div class="panel-body"><div class="network-toolbar"><div class="form-field"><label for="network-user">Focus person</label><select id="network-user"><option value="">All people</option></select></div><div class="network-selection subtle" id="network-selection">Select a node for details.</div></div><div id="network-visual"><div class="loading"><span class="spinner"></span>Loading graph from the C engine…</div></div><div class="network-legend" id="network-legend"></div></div></section>`;
  const visual = container.querySelector("#network-visual");
  let graph;
  let adjacency = new Map();
  let selectedId = "";
  const selection = container.querySelector("#network-selection");
  const select = container.querySelector("#network-user");
  const describe = (id) => {
    selectedId = id;
    select.value = id;
    const person = graph.nodes.find((node) => String(node.id) === id);
    selection.textContent = person ? `${person.name} · user #${person.id} · ${person.friendCount} friends · community ${person.groupId}` : "Select a node for details.";
    visual.querySelectorAll(".network-edge").forEach((edge) => {
      const connected = id && (edge.dataset.source === id || edge.dataset.target === id);
      edge.classList.toggle("highlighted", Boolean(connected));
    });
    const friends = adjacency.get(Number(id)) || new Set();
    visual.querySelectorAll(".network-node").forEach((node) => {
      const chosen = node.dataset.node === id;
      node.classList.toggle("selected", chosen);
      node.classList.toggle("dimmed", Boolean(id && !chosen && !friends.has(Number(node.dataset.node))));
      node.querySelector("circle").setAttribute("r", chosen ? "17" : "13");
    });
  };
  const load = async () => {
    showLoading(visual);
    try {
      graph = await api("/graph");
      if (!container.querySelector("#network-user")) return;
      adjacency = new Map(graph.nodes.map((node) => [node.id, new Set()]));
      graph.edges.forEach(({ source, target }) => {
        adjacency.get(source)?.add(target);
        adjacency.get(target)?.add(source);
      });
      container.querySelector("#network-count").textContent = `${graph.nodes.length} users · ${graph.edges.length} friendships`;
      select.innerHTML = `<option value="">All people</option>${graph.nodes.map((node) => `<option value="${node.id}">${escapeHtml(node.name)} · #${node.id}</option>`).join("")}`;
      const groupIds = Array.from(new Set(graph.nodes.map((node) => node.groupId)));
      container.querySelector("#network-legend").innerHTML = groupIds.map((id, index) => `<span><i style="background:${colors[index % colors.length]}"></i>Community ${id}</span>`).join("");
      visual.innerHTML = renderGraph(graph);
      selection.textContent = "Select a node for details.";
      visual.querySelectorAll(".network-node").forEach((node) => {
        node.addEventListener("click", () => describe(node.dataset.node));
        node.addEventListener("keydown", (event) => {
          if (event.key === "Enter" || event.key === " ") {
            event.preventDefault();
            describe(node.dataset.node);
          }
        });
      });
      visual.querySelector("svg").addEventListener("click", (event) => {
        if (event.target === event.currentTarget) describe("");
      });
    } catch (error) {
      visual.innerHTML = `<div class="empty-state">${escapeHtml(error.message)}</div>`;
      showToast(error.message, true);
    }
  };
  select.addEventListener("change", () => describe(select.value));
  container.querySelector("#network-refresh").addEventListener("click", load);
  await load();
}
