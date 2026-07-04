
const API_BASE_URL = window.API_BASE_URL || "http://127.0.0.1:5001";
const ITEMS_PER_PAGE = 20;

const state = {
    allData: [],
    currentPage: 1,
    lastRate: 1,
    sort: "date-desc",
    sortAscending: {
        date: false,
        time: false,
        money: false,
        total: false,
    },
    exchangeToUsd: false,
    rankingMode: false,
};

const elements = {
    userName: document.querySelector("#user-name"),
    table: document.querySelector("#money-table"),
    pagination: document.querySelector("#pagination"),
    exchangeButton: document.querySelector("#exchange-button"),
    rankButton: document.querySelector("#rank-button"),
    costPanel: document.querySelector("#cost-panel"),
    namePanel: document.querySelector("#name-panel"),
    costInput: document.querySelector("#cost-input"),
    nameInput: document.querySelector("#name-input"),
    costSubmit: document.querySelector("#cost-submit"),
    nameSubmit: document.querySelector("#name-submit"),
    sortButtons: document.querySelectorAll("[data-sort]"),
    panelButtons: document.querySelectorAll("[data-panel]"),
};

async function fetchMoneyData(options = {}) {
    const params = new URLSearchParams({
        state: state.sort,
        value: options.value || "",
        change: options.change || "",
    });

    const response = await fetch(`${API_BASE_URL}/get_money_data?${params.toString()}`);
    if (!response.ok) {
        throw new Error(`API request failed: ${response.status}`);
    }

    const result = await response.json();
    state.allData = result.data || [];
    state.lastRate = result.rate || 1;
    elements.userName.textContent = `你好! ${result.name || ""}`;

    if (result.table_state === "fall") {
        alert("輸入金額已超出已有金額，請重新確認後再輸入");
    }

    render(state.lastRate);
}

function render(rate) {
    updateButtonState();

    if (state.sort === "rank") {
        renderRankingTable(rate);
        elements.pagination.innerHTML = "";
        return;
    }

    renderMoneyTable(rate);
    renderPagination();
}

function renderRankingTable(rate) {
    const rows = state.allData
        .map((item, index) => {
            const total = formatMoney(item.total, rate);
            return `<tr><td>${index + 1}</td><td>${escapeHtml(item.name)}</td><td>${total}</td></tr>`;
        })
        .join("");

    elements.table.innerHTML = `
        <tr><th>排行</th><th>名稱</th><th>總額</th></tr>
        ${rows}
    `;
}

function renderMoneyTable(rate) {
    const start = (state.currentPage - 1) * ITEMS_PER_PAGE;
    const pageData = state.allData.slice(start, start + ITEMS_PER_PAGE);
    const rows = pageData
        .map((item) => {
            return `
                <tr>
                    <td>${escapeHtml(item.date)}</td>
                    <td>${escapeHtml(item.time)}</td>
                    <td>${formatMoney(item.money, rate)}</td>
                    <td>${formatMoney(item.total, rate)}</td>
                </tr>
            `;
        })
        .join("");

    elements.table.innerHTML = `
        <tr><th>日期</th><th>時間</th><th>金額更動</th><th>總金額</th></tr>
        ${rows}
    `;
}

function renderPagination() {
    const totalPages = Math.max(1, Math.ceil(state.allData.length / ITEMS_PER_PAGE));
    const buttons = [];

    buttons.push(pageButton("←", state.currentPage - 1, state.currentPage === 1));
    for (let page = 1; page <= totalPages; page += 1) {
        buttons.push(pageButton(page, page, state.currentPage === page));
    }
    buttons.push(pageButton("→", state.currentPage + 1, state.currentPage === totalPages));

    elements.pagination.innerHTML = buttons.join("");
    elements.pagination.querySelectorAll("[data-page]").forEach((button) => {
        button.addEventListener("click", () => changePage(Number(button.dataset.page)));
    });
}

function pageButton(label, page, disabled) {
    return `<button class="page-button" data-page="${page}" type="button" ${disabled ? "disabled" : ""}>${label}</button>`;
}

function changePage(newPage) {
    const totalPages = Math.max(1, Math.ceil(state.allData.length / ITEMS_PER_PAGE));
    if (newPage < 1 || newPage > totalPages) {
        return;
    }

    state.currentPage = newPage;
    renderMoneyTable(state.lastRate);
    renderPagination();
}

function formatMoney(value, rate) {
    const amount = Number(value || 0);
    if (state.exchangeToUsd) {
        return `${(amount / rate).toFixed(2)}$`;
    }
    return `${amount}$`;
}

function updateButtonState() {
    elements.sortButtons.forEach((button) => {
        const sortKey = button.dataset.sort;
        const ascState = `${sortKey}`;
        const descState = `${sortKey}-desc`;
        const isActive = state.sort === ascState || state.sort === descState;
        const arrow = state.sort === ascState ? "▲" : state.sort === descState ? "▼" : "";
        button.textContent = `${buttonText(sortKey)}${arrow}`;
        button.classList.toggle("active", isActive);
    });

    elements.exchangeButton.textContent = state.exchangeToUsd ? "轉匯(美)" : "轉匯(台)";
    elements.rankButton.classList.toggle("active", state.sort === "rank");
}

function buttonText(sortKey) {
    return {
        date: "依日期",
        time: "依時間",
        money: "依單筆",
        total: "依總額",
    }[sortKey];
}

function togglePanel(panelName) {
    const selectedPanel = panelName === "cost" ? elements.costPanel : elements.namePanel;
    const otherPanel = panelName === "cost" ? elements.namePanel : elements.costPanel;

    otherPanel.classList.add("hidden");
    selectedPanel.classList.toggle("hidden");
}

function escapeHtml(value) {
    return String(value ?? "")
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;")
        .replaceAll("'", "&#039;");
}

function showError(error) {
    console.error(error);
    elements.table.innerHTML = `<tr><td class="error">資料讀取失敗，請確認後端服務是否啟動。</td></tr>`;
}

function bindEvents() {
    elements.sortButtons.forEach((button) => {
        button.addEventListener("click", () => {
            const sortKey = button.dataset.sort;
            state.rankingMode = false;
            state.sortAscending[sortKey] = !state.sortAscending[sortKey];
            state.sort = state.sortAscending[sortKey] ? sortKey : `${sortKey}-desc`;
            state.currentPage = 1;
            fetchMoneyData().catch(showError);
        });
    });

    elements.exchangeButton.addEventListener("click", () => {
        state.exchangeToUsd = !state.exchangeToUsd;
        fetchMoneyData().catch(showError);
    });

    elements.rankButton.addEventListener("click", () => {
        state.rankingMode = !state.rankingMode;
        state.sort = state.rankingMode ? "rank" : "date-desc";
        state.currentPage = 1;
        fetchMoneyData().catch(showError);
    });

    elements.panelButtons.forEach((button) => {
        button.addEventListener("click", () => togglePanel(button.dataset.panel));
    });

    elements.costSubmit.addEventListener("click", () => {
        const value = elements.costInput.value;
        elements.costInput.value = "";
        fetchMoneyData({ value, change: "cost" }).catch(showError);
    });

    elements.nameSubmit.addEventListener("click", () => {
        const value = elements.nameInput.value;
        elements.nameInput.value = "";
        fetchMoneyData({ value, change: "name" }).catch(showError);
    });
}

bindEvents();
fetchMoneyData().catch(showError);
setInterval(() => fetchMoneyData().catch(showError), 3000);
