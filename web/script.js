let allData = []; // 儲存後端回傳的完整數據
let currentPage = 1; // 當前頁碼
const itemsPerPage = 20; // 每頁顯示數據筆數
let date_s = 0, time_s = 0, money_s = 0, total_s = 0, change_s = 0, rank = 0, state = 'date-desc';
const input_name = document.getElementById('div_input-name');
const input_cost = document.getElementById('div_input-cost');
input_cost.style.display = 'none';
input_name.style.display = 'none';
function fetchMoneyData(state, value = '', change = '') {
    fetch(`http://192.168.0.122:5001/get_money_data?state=${state}&value=${value}&change=${change}`)  // 請求後端 API
        .then(response => response.json())  // 解析 JSON 資料
        .then(result => {
            const data = result.data;
            allData = result.data;
            const sql_name = result.name;
            const rate = result.rate || 1;
            const table_state = result.table_state;
            let money_code = "";
            document.querySelector("#user_name").innerHTML = `你好! ${sql_name}`;
            if(table_state === 'fall'){
                alert("輸入金額已超出已有金額 請重新確認後再輸入");
            }
            if(state === 'rank'){
                for (let i = 0; i < data.length; i++) {
                    const utc_total = change_s ? (data[i].total / rate).toFixed(2) : data[i].total;
                    money_code += `
                        <tr>
                            <td>${i + 1}</td>
                            <td>${data[i].name}</td>
                            <td>${utc_total}$</td>
                        </tr>`;
                }
                document.querySelector(".save_data").innerHTML = `
                    <tr>
                        <td>排行</td>
                        <td>名稱</td>
                        <td>總額</td>
                    </tr>` + money_code;
                document.getElementById("pagination").innerHTML = '';
            }
            else{
                rank = 0;
                const start = (currentPage - 1) * itemsPerPage;
                const end = start + itemsPerPage;
                const pageData = allData.slice(start, end); // 當前頁的數據
                
                let money_code = `
                    <tr>
                        <td>日期</td>
                        <td>時間</td>
                        <td>金額更動</td>
                        <td>總金額</td>
                    </tr>`;
                pageData.forEach(item => {
                    const utc_money = change_s ? (item.money / rate).toFixed(2) : item.money;
                    const utc_total = change_s ? (item.total / rate).toFixed(2) : item.total;
                    money_code += `
                        <tr>
                            <td>${item.date}</td>
                            <td>${item.time}</td>
                            <td>${utc_money}$</td>
                            <td>${utc_total}$</td>
                        </tr>`;
                });
                document.querySelector(".save_data").innerHTML = money_code;
                renderPagination();
            }
        })
}
function startAutoRefresh() {
    setInterval(() => {fetchMoneyData(state);}, 3000);
}
function switchinput(change_use){
    if(change_use === 'cost'){
        if (input_cost.style.display === 'none') {
            input_name.style.display = 'none';
            input_cost.style.display = 'block'; // 顯示
        } 
        else {
            input_cost.style.display = 'none'; // 隱藏
        }
    }
    else if(change_use === 'name'){
        if (input_name.style.display === 'none') {
            input_cost.style.display = 'none';
            input_name.style.display = 'block'; // 顯示
        } 
        else {
            input_name.style.display = 'none'; // 隱藏
        }
    }
}
function button_text(button_name){
    document.querySelector('#button-date').innerText = 
        button_name === 'date' ? '依日期▲' : 
        button_name === 'date-desc' ? '依日期▼' : 
        '依日期';
    document.querySelector('#button-time').innerText = 
        button_name === 'time' ? '依時間▲' : 
        button_name === 'time-desc' ? '依時間▼' : 
        '依時間';
    document.querySelector('#button-money').innerText = 
        button_name === 'money' ? '依單筆▲' : 
        button_name === 'money-desc' ? '依單筆▼' : 
        '依單筆';
    document.querySelector('#button-total').innerText = 
        button_name === 'total' ? '依總額▲' : 
        button_name === 'total-desc' ? '依總額▼' : 
        '依總額';
}
function renderPagination() {
    const totalPages = Math.ceil(allData.length / itemsPerPage);
    let paginationButtons = '';
    paginationButtons += `
        <button class="pagebutton" onclick="
            fetchMoneyData(state);
            changePage(currentPage - 1);
        " ${currentPage === 1 ? 'disabled' : ''}>&#8592;</button>`;
    for (let i = 1; i <= totalPages; i++) {//每個頁碼按鈕
        paginationButtons += `
            <button class="pagebutton" onclick="
                fetchMoneyData(state);
                changePage(${i});
            " ${currentPage === i ? 'disabled' : ''}>${i}</button>`;
    }
    paginationButtons += `
        <button class="pagebutton" onclick="
            fetchMoneyData(state);
            changePage(currentPage + 1);
        " ${currentPage === totalPages ? 'disabled' : ''}>&#8594;</button>`;
    document.getElementById("pagination").innerHTML = paginationButtons;
}
function changePage(newPage) {// 更改頁碼
    const totalPages = Math.ceil(allData.length / itemsPerPage);
    if (newPage >= 1 && newPage <= totalPages) {
        currentPage = newPage;
        renderTable();
        renderPagination();
    }
}
fetchMoneyData(state);
button_text(state);
startAutoRefresh();