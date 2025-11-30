from flask import Flask, jsonify, request
from flask_cors import CORS
import mysql.connector
import requests
import cv2
import numpy as np
import os
from threading import Thread
import serial

app = Flask(__name__) 
CORS(app)

user_num = 1

# 鏡頭設定
cap = cv2.VideoCapture(0)
# cap = cv2.VideoCapture("http://192.168.21.85/stream")

# 人臉識別設定
model_path = "my.yml" #紀錄
faceCascade = cv2.CascadeClassifier("face_detect.xml") #偵測模型
recognizer = cv2.face.LBPHFaceRecognizer_create() #辨識模型

# 序列埠設定
ser = serial.Serial('/dev/cu.wchusbserial54E20196661', 115200, timeout=2)
ser.bytesize = serial.EIGHTBITS
ser.stopbits = serial.STOPBITS_ONE
ser.parity = serial.PARITY_NONE

try:#一開始有檔案
    recognizer.read(model_path)
    loading = False
    identify = True
    labels = recognizer.getLabels().flatten()
    person_num = max(labels) + 1
except:#一開始沒檔案
    loading = False
    identify = False
    person_num = 1
    print("目前暫無模型 請按下空白鍵開始建立模型")

loading_faces = []  # 儲存人臉的數據
ids = []  # 對應人臉的 ID

# 取得匯率
def get_exchange_rate(base_currency, target_currency):
    url = f"https://open.er-api.com/v6/latest/{base_currency}"
    response = requests.get(url)
    data = response.json()
    if response.status_code == 200:
        return data['rates'].get(target_currency, None)
    else:
        print("Error fetching exchange rates:", data)
        return None

# Flask API：處理金錢數據
@app.route('/get_money_data', methods=['GET'])
def get_money_data():
    change = request.args.get('change')
    state = request.args.get('state', 'date')
    value = request.args.get('value')
    state_options = {
        'time': "ORDER BY `time`",
        'time-desc': "ORDER BY `time` DESC",
        'money': "ORDER BY `money`",
        'money-desc': "ORDER BY `money` DESC",
        'total': "ORDER BY `total`",
        'total-desc': "ORDER BY `total` DESC",
        'date' : "ORDER BY `date`",
        'date-desc': "ORDER BY `date` DESC, `time` DESC",
        'rank' : "rank"
    }
    get_state = state_options.get(state, "ORDER BY `date` DESC")
    try:
        connection = mysql.connector.connect(
            host="localhost",
            port="3306",
            user="root",
            password="951023pp",
            database="data"
        )
        data = []
        rate = get_exchange_rate("USD", "TWD") or "Unavailable"
        cursor = connection.cursor()
        cursor.execute("SELECT * FROM `user_name`;")
        sql_name = cursor.fetchall()
        sql_name = sql_name[user_num - 1]
        original_name = sql_name[0]
        user_name = sql_name[1]
        table_state = 'success'
        #esp32存錢
        if(ser.in_waiting > 0):
            try:
                esp32_save = ser.readline().decode('utf-8').strip()
            except Exception as e:
                print(f"無法讀取儲存金額 錯誤碼: {e}")
            try:
                cursor.execute(f"SELECT * FROM `{original_name}` ORDER BY `date`;")
                total_num = cursor.fetchall()
                total_num = total_num[len(total_num) - 1]
                total_num = total_num[4]
            except:
                total_num = 0
            if(int(value) <= total_num):
                cursor.execute(f"SET @save = {esp32_save};")
                cursor.execute(f"SET @total = (SELECT IFNULL(SUM(money), 0) FROM `{original_name}`)")
                cursor.execute(f"INSERT INTO `{original_name}` (`date`, `time`, `money`, `total`) \
                                VALUES (NOW(), NOW(), @save, @total + @save);")
            else:
                table_state = 'fall'
        #網頁存錢
        if(change == "cost"):
            try:
                cursor.execute(f"SELECT * FROM `{original_name}` ORDER BY `date`;")
                total_num = cursor.fetchall()
                total_num = total_num[len(total_num) - 1]
                total_num = total_num[4]
            except:
                total_num = 0
            if(int(value) <= total_num):
                cursor.execute(f"SET @save = -{value};")
                cursor.execute(f"SET @total = (SELECT IFNULL(SUM(money), 0) FROM `{original_name}`)")
                cursor.execute(f"INSERT INTO `{original_name}` (`date`, `time`, `money`, `total`) \
                                VALUES (NOW(), NOW(), @save, @total + @save);")
            else:
                table_state = 'fall'    
        #改名
        if(change == "name"):
            cursor.execute(f"UPDATE `user_name` SET `new_name` = '{value}' where `original_name` = '{original_name}';")
            user_name = value
        #排行
        if(get_state == "rank"):
            cursor.execute("SHOW TABLES;")
            records = [row[0] for row in cursor.fetchall()]
            for r in records:
                if(r == 'user_name'):
                    continue
                cursor.execute(f"SELECT SUM(`money`) FROM `{r}`;")
                total = cursor.fetchone()
                total = total[0]
                cursor.execute(f"SELECT `new_name` FROM `user_name` WHERE `original_name` = '{r}';")
                name = cursor.fetchall()
                name = name[0]
                data.append({
                    "name": name[0],
                    "total": total if total else 0
                })
            data.sort(key=lambda x: x["total"], reverse=True)
        #各種排序
        else:
            cursor.execute(f"SELECT * FROM `{original_name}` {get_state};")
            records = cursor.fetchall()
            for r in records:
                date = str(r[1])
                time = str(r[2])
                money = r[3]
                total = r[4]
                data.append({
                    "date": date,
                    "time": time,
                    "money": money,
                    "total": total
                })
        #回傳前端數值
        return jsonify({"data": data, "rate": rate, "name": user_name, "table_state": table_state})
    except mysql.connector.Error as err:
        return jsonify({"MySQL通訊錯誤 錯誤碼:": str(err)}), 500
    finally:
        connection.commit()
        if cursor:
            cursor.close()
        if connection:
            connection.close()

# 人臉訓練與識別
def face_recognition():
    global loading, identify, person_num, loading_faces, ids, user_num
    while True:
        if loading:#掃描模型
            print("開始辨識 請對準人臉")
            loading_faces = []  # 儲存人臉的數據
            ids = []    # 對應人臉的 ID
            for i in range(1,50):
                ret, frame = cap.read()
                if ret != 1:
                    print('沒抓到畫面')
                    break
                # frame = cv2.rotate(frame, cv2.ROTATE_90_COUNTERCLOCKWISE)
                gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                img_np = np.array(gray,'uint8') 
                faceRect = faceCascade.detectMultiScale(gray)
                if len(faceRect) > 0:
                    for (x, y, w, h) in faceRect[:1]:# 捕获第一张人脸
                        loading_faces.append(img_np[y:y+h, x:x+w])
                        ids.append(int(person_num))
            print(f"最終訓練面容數: {len(loading_faces)}, ID總數: {ids}")# Debug 输出捕获结果
            if len(loading_faces) > 40:
                if os.path.exists(model_path):
                    try:
                        recognizer.read(model_path)
                        recognizer.update(loading_faces, np.array(ids))
                        print("模型更新完成 您的編號為NO.",person_num)
                    except:
                        os.remove(model_path)
                        recognizer.train(loading_faces, np.array(ids))
                        print("模型建立完成 您的編號為NO.",person_num)
                else:
                    recognizer.train(loading_faces, np.array(ids))
                    print("模型建立完成 您的編號為NO.",person_num)
                recognizer.save(model_path)
                person_num = person_num + 1
                identify = True
            else:
                print("失敗 人臉捕獲數:",len(loading_faces))
            loading = False
        if identify:#人臉辨識
            ret, frame = cap.read()
            if ret != 1:
                print('沒抓到畫面')
                break
            # frame = cv2.rotate(frame, cv2.ROTATE_90_COUNTERCLOCKWISE)
            gray = cv2.cvtColor(frame,cv2.COLOR_BGR2GRAY)  # 轉換成黑白
            faces = faceCascade.detectMultiScale(gray)  # 追蹤人臉 ( 目的在於標記出外框 )
            name = {# 建立姓名和 id 的對照表
                '1':'3',
                '2':'2',
                '3':'3',
                '4':'4',
                '5':'5',
                '6':'6'
            }
            for(x,y,w,h) in faces:#依序判斷每張臉屬於哪個 id
                idnum,confidence = recognizer.predict(gray[y:y+h,x:x+w])#取出id號碼以及信心指數confidence
                if confidence < 30:#如果信心指數小於30
                    text = name.get(str(idnum), "error")
                    ser.write(text.encode('utf-8'))
                    user_num = int(text)
            key = cv2.waitKey(1) & 0xFF
            if key == ord(' '):
                loading = True
        else:#一開始沒模型 且捕捉失敗
            ret, frame = cap.read()
            if ret != 1:
                print('沒抓到畫面')
                break
            key = cv2.waitKey(1) & 0xFF
            if key == ord(' '):
                loading = True
    cap.release()#清理資源
    cv2.destroyAllWindows()

if __name__ == '__main__':
    # 啟動人臉識別的函數（在獨立線程中運行）
    recognition_thread = Thread(target=face_recognition)
    recognition_thread.daemon = True
    recognition_thread.start()
    
    # 啟動Flask API（在主線程中運行）
    app.run(host='0.0.0.0', port=5001, debug=True)
