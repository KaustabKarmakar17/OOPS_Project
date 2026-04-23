(function () {
  "use strict";

  let currentPage = "dashboard";
  let sidebarCollapsed = localStorage.getItem("sidebarCollapsed") === "true";

  let allStudents = [];
  let filteredStudents = [];
  let currentPageTable = 1;
  const rowsPerPage = 100;

  const sidebar = document.getElementById("sidebar");
  const pages = document.querySelectorAll(".page");
  const navItems = document.querySelectorAll(".nav-item");
  const collapseBtn = document.getElementById("collapseSidebarBtn");
  const globalSearch = document.getElementById("globalSearch");
  const tableBody = document.getElementById("tableBody");
  const paginationDiv = document.getElementById("pagination");
  const tableSearch = document.getElementById("tableSearch");
  const sortSelect = document.getElementById("sortSelect");

  function showToast(message, type = "info") {
    const container =
      document.getElementById("toastContainer") ||
      (() => {
        let d = document.createElement("div");
        d.id = "toastContainer";
        d.className = "toast-container";
        document.body.appendChild(d);
        return d;
      })();
    const toast = document.createElement("div");
    toast.className = `toast ${type}`;
    toast.textContent = message;
    container.appendChild(toast);
    setTimeout(() => toast.remove(), 3000);
  }

  function animateCounter(el, start, end, duration) {
    let startTime;
    const step = (timestamp) => {
      if (!startTime) startTime = timestamp;
      const progress = Math.min((timestamp - startTime) / duration, 1);
      el.textContent = (progress * end).toFixed(end % 1 ? 2 : 0);
      if (progress < 1) requestAnimationFrame(step);
    };
    requestAnimationFrame(step);
  }

  // --- API FETCH ---
  async function fetchUsersAPI() {
    try {
      const res = await fetch("http://localhost:18080/users");
      if (!res.ok) throw new Error("Network response was not ok");
      const json = await res.json();
      if (json.success && json.data && json.data.users) {
        allStudents = json.data.users;
        filteredStudents = [...allStudents];
        return true;
      }
    } catch (error) {
      console.error("API Error:", error);
      showToast("Failed to fetch live data from backend", "error");
    }
    return false;
  }

  function updateMetrics() {
    const grid = document.getElementById("metricsGrid");
    if (!grid) return;
    if (allStudents.length === 0) {
      grid.innerHTML =
        '<div class="skeleton" style="height:120px;grid-column:span 4"></div>'.repeat(
          4,
        );
      return;
    }

    const total = allStudents.length;
    const avgCg =
      allStudents.reduce((a, b) => a + (b.actual_cg || 0), 0) / total || 0;
    const maxCg = Math.max(...allStudents.map((s) => s.actual_cg || 0), 0);
    const failRate = (
      (allStudents.filter((s) => s.fail_count > 0).length / total) *
      100
    ).toFixed(1);

    grid.innerHTML = [
      { label: "Total Students", value: total },
      { label: "Average CGPA", value: avgCg.toFixed(2) },
      { label: "Highest CGPA", value: maxCg.toFixed(2) },
      { label: "Failure Rate", value: failRate + "%" },
    ]
      .map(
        (m) => `
      <div class="metric-card">
        <div class="metric-label">${m.label}</div>
        <div class="metric-value" data-target="${m.value}">0</div>
        <div class="trend">Live Data</div>
      </div>
    `,
      )
      .join("");

    document.querySelectorAll(".metric-value").forEach((el) => {
      animateCounter(el, 0, parseFloat(el.dataset.target) || 0, 800);
    });
  }

  function getBadgeClass(trend) {
    if (trend === "Improving") return "badge-improving";
    if (trend === "Declining") return "badge-declining";
    return "badge-stable";
  }

  function renderTable() {
    if (!tableBody) return;
    const searchTerm = (tableSearch?.value || "").toLowerCase();

    let filtered = allStudents.filter((s) =>
      s.registration_no?.toLowerCase().includes(searchTerm),
    );

    const sort = sortSelect?.value || "cgpa_desc";
    if (sort === "cgpa_desc")
      filtered.sort((a, b) => (b.actual_cg || 0) - (a.actual_cg || 0));
    else if (sort === "cgpa_asc")
      filtered.sort((a, b) => (a.actual_cg || 0) - (b.actual_cg || 0));
    else if (sort === "avg_gp_desc")
      filtered.sort((a, b) => (b.avg_gp || 0) - (a.avg_gp || 0));
    else if (sort === "avg_gp_asc")
      filtered.sort((a, b) => (a.avg_gp || 0) - (b.avg_gp || 0));

    filteredStudents = filtered;
    const start = (currentPageTable - 1) * rowsPerPage;

    tableBody.innerHTML = filtered
      .slice(start, start + rowsPerPage)
      .map((s) => {
        const cgClass =
          s.actual_cg < 6 ? "cgpa-low" : s.actual_cg >= 8.5 ? "cgpa-high" : "";
        const trendBadge = `<span class="badge ${getBadgeClass(s.gp_trend)}">${s.gp_trend || "Unknown"}</span>`;

        return `<tr class="${cgClass}">
        <td>${s.registration_no || "N/A"}</td>
        <td class="font-medium">${(s.actual_cg || 0).toFixed(2)}</td>
        <td>${(s.avg_gp || 0).toFixed(2)}</td>
        <td class="text-muted">${(s.last_gp || 0).toFixed(1)}</td>
        <td>${(s.performance_variance || 0).toFixed(3)}</td>
        <td>${s.fail_count || 0}</td>
        <td>${trendBadge}</td>
      </tr>`;
      })
      .join("");

    renderPagination(filtered.length);
  }

  function renderPagination(total) {
    if (!paginationDiv) return;
    const pages = Math.ceil(total / rowsPerPage);
    paginationDiv.innerHTML = Array.from(
      { length: pages },
      (_, i) =>
        `<button class="page-btn ${i + 1 === currentPageTable ? "active" : ""}" data-page="${i + 1}">${i + 1}</button>`,
    ).join("");
    document.querySelectorAll(".page-btn").forEach((b) =>
      b.addEventListener("click", (e) => {
        currentPageTable = +e.target.dataset.page;
        renderTable();
      }),
    );
  }

  function drawCharts() {
    if (allStudents.length === 0) return;

    const barCanvas = document.getElementById("cgpaBarChart");
    if (barCanvas) {
      const ctx = barCanvas.getContext("2d");
      const width = barCanvas.parentElement.clientWidth - 40;
      barCanvas.width = width;
      barCanvas.height = 200;

      const ranges = [0, 0, 0, 0, 0];
      allStudents.forEach((s) => {
        const cg = s.actual_cg || 0;
        if (cg < 5) ranges[0]++;
        else if (cg < 6) ranges[1]++;
        else if (cg < 7) ranges[2]++;
        else if (cg < 8) ranges[3]++;
        else ranges[4]++;
      });

      const maxCount = Math.max(...ranges, 1);
      const barWidth = (width - 100) / 5;
      ctx.clearRect(0, 0, width, 200);

      ranges.forEach((count, i) => {
        const barHeight = (count / maxCount) * 140;
        const x = 60 + i * barWidth,
          y = 200 - 30 - barHeight;
        ctx.fillStyle = "#0f172a";
        ctx.fillRect(x, y, barWidth - 10, barHeight);
        ctx.font = "12px Inter";
        ctx.textAlign = "center";
        ctx.fillText(
          ["<5", "5-6", "6-7", "7-8", "8+"][i],
          x + (barWidth - 10) / 2,
          190,
        );
      });
    }

    const donutCanvas = document.getElementById("passFailDonut");
    if (donutCanvas) {
      const ctx = donutCanvas.getContext("2d");
      donutCanvas.width = 200;
      donutCanvas.height = 200;
      const pass = allStudents.filter((s) => s.fail_count === 0).length,
        fail = allStudents.length - pass;

      ctx.clearRect(0, 0, 200, 200);
      ctx.beginPath();
      ctx.fillStyle = "#1e293b";
      ctx.arc(100, 100, 70, 0, (pass / allStudents.length) * Math.PI * 2);
      ctx.lineTo(100, 100);
      ctx.fill();
      ctx.beginPath();
      ctx.fillStyle = "#e2e8f0";
      ctx.arc(
        100,
        100,
        70,
        (pass / allStudents.length) * Math.PI * 2,
        Math.PI * 2,
      );
      ctx.lineTo(100, 100);
      ctx.fill();
      ctx.beginPath();
      ctx.fillStyle = "#ffffff";
      ctx.arc(100, 100, 40, 0, Math.PI * 2);
      ctx.fill();
      ctx.fillStyle = "#0f172a";
      ctx.font = "bold 14px Inter";
      ctx.textAlign = "center";
      ctx.fillText(`${pass} Pass`, 100, 95);
      ctx.fillText(`${fail} Fail`, 100, 115);
    }

    const lineCanvas = document.getElementById("trendLineCanvas");
    if (lineCanvas) {
      const ctx = lineCanvas.getContext("2d");
      const width = lineCanvas.parentElement.clientWidth - 40;
      lineCanvas.width = width;
      lineCanvas.height = 150;

      const sorted = [...allStudents].sort(
        (a, b) => (a.actual_cg || 0) - (b.actual_cg || 0),
      );
      ctx.beginPath();
      ctx.strokeStyle = "#0f172a";
      ctx.lineWidth = 2;
      sorted.forEach((s, i) => {
        const x = 50 + (i / Math.max(sorted.length - 1, 1)) * (width - 100);
        const y = 150 - 30 - ((s.actual_cg || 0) / 10) * 120;
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      });
      ctx.stroke();
    }
  }

  function switchPage(pageId) {
    pages.forEach((p) => p.classList.remove("active"));
    document.getElementById(`page-${pageId}`)?.classList.add("active");
    navItems.forEach((n) =>
      n.classList.toggle("active", n.dataset.page === pageId),
    );
    if (pageId === "dashboard") updateMetrics();
    if (pageId === "students") renderTable();
    if (pageId === "analytics") setTimeout(drawCharts, 50);
  }

  // --- UI EVENT LISTENERS ---
  if (sidebarCollapsed && sidebar) sidebar.classList.add("collapsed");

  collapseBtn?.addEventListener("click", () => {
    sidebar.classList.toggle("collapsed");
    localStorage.setItem(
      "sidebarCollapsed",
      sidebar.classList.contains("collapsed"),
    );
  });

  navItems.forEach((item) =>
    item.addEventListener("click", (e) => {
      e.preventDefault();
      switchPage(item.dataset.page);
    }),
  );

  let searchTimeout;
  globalSearch?.addEventListener("input", (e) => {
    clearTimeout(searchTimeout);
    searchTimeout = setTimeout(() => {
      const query = e.target.value.toLowerCase().trim();
      if (!query) return;
      const found = allStudents.find((s) =>
        s.registration_no?.toLowerCase().includes(query),
      );
      if (found) {
        showToast(
          `Found: ${found.registration_no} | CGPA: ${found.actual_cg}`,
          "success",
        );
        if (tableSearch) tableSearch.value = query;
        switchPage("students");
      } else showToast("No student found", "error");
    }, 300);
  });

  tableSearch?.addEventListener("input", renderTable);
  sortSelect?.addEventListener("change", renderTable);

  // --- SGPA SIMULATOR & ML PREDICTION ---
  const generateBtn = document.getElementById("generateSemBtn");
  if (generateBtn) {
    generateBtn.addEventListener("click", () => {
      // Updated clamping: Maximum 7 semesters
      const count = Math.max(
        1,
        Math.min(7, parseInt(document.getElementById("semCount").value) || 3),
      );
      document.getElementById("semCount").value = count;
      const container = document.getElementById("sgpaInputsContainer");

      container.innerHTML = Array.from(
        { length: count },
        (_, i) => `
        <div class="input-group">
          <label class="text-sm font-medium">Sem ${i + 1} SGPA</label>
          <input type="number" step="0.01" min="0" max="10" class="form-input sgpa-input" value="8.0" id="sgpa${i}">
        </div>
      `,
      ).join("");

      document.getElementById("sgpaPredictionOutput").style.display = "none";
    });
    setTimeout(() => generateBtn.click(), 100);
  }

  document.addEventListener("input", (e) => {
    if (e.target.classList.contains("sgpa-input")) {
      const inputs = document.querySelectorAll(".sgpa-input");
      const values = Array.from(inputs).map((input) =>
        Math.max(0, Math.min(10, parseFloat(input.value) || 0)),
      );

      const avg = values.reduce((a, b) => a + b, 0) / values.length;
      const trend =
        values.length > 1 ? values[values.length - 1] - values[0] : 0;

      document.getElementById("avgSgpaDisplay").textContent = avg.toFixed(2);
      const trendDisplay = document.getElementById("sgpaTrendDisplay");
      trendDisplay.innerHTML =
        trend > 0.1
          ? "📈 Improving"
          : trend < -0.1
            ? "📉 Declining"
            : "➡️ Stable";

      document.getElementById("sgpaPredictionOutput").style.display = "none";
    }
  });

  const predictSgpaBtn = document.getElementById("predictSgpaBtn");
  if (predictSgpaBtn) {
    predictSgpaBtn.addEventListener("click", async () => {
      const values = Array.from(document.querySelectorAll(".sgpa-input")).map(
        (i) => parseFloat(i.value) || 0,
      );
      if (values.length === 0)
        return showToast("Generate semester inputs first", "error");

      const out = document.getElementById("sgpaPredictionOutput");
      out.style.display = "block";

      try {
        const res = await fetch("http://localhost:18080/predict", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ sems: values }),
        });
        if (!res.ok) throw new Error("API Error");

        const json = await res.json();
        if (json.success && json.data) {
          const d = json.data;
          document.getElementById("predCgpaVal").textContent =
            d.predicted_cg_lr.toFixed(2);

          const catBadge = document.getElementById("predCategory");
          catBadge.textContent = d.lr_category;
          catBadge.className = `badge ${d.lr_category === "Critical" ? "badge-declining" : d.lr_category.includes("Average") ? "badge-stable" : "badge-improving"}`;

          document.getElementById("predConfidenceBar").style.width =
            `${d.data_confidence}%`;
          document.getElementById("predAgreement").textContent =
            d.agreement_confidence;
          document.getElementById("predConfidenceText").textContent =
            `${Math.round(d.data_confidence)}%`;

          showToast(
            `Prediction: ${d.predicted_cg_lr.toFixed(2)} CGPA`,
            "success",
          );
        }
      } catch (err) {
        showToast("Prediction failed. Check backend.", "error");
      }
    });
  }

  // --- BOOTSTRAP ---
  async function initialize() {
    await fetchUsersAPI();
    updateMetrics();
    renderTable();
    switchPage(window.location.hash.slice(1) || "dashboard");
    setTimeout(drawCharts, 100);
  }

  if (document.readyState === "loading")
    document.addEventListener("DOMContentLoaded", initialize);
  else initialize();
})();
