import sqlite3
from datetime import datetime
import my_camera

def make_db():
    try:
        conn = sqlite3.connect("database.db")
        cursor = conn.cursor()

        # MEMBER 테이블 생성
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS MEMBER (
            student_id INTEGER PRIMARY KEY,
            name TEXT,
            rfid_tag_id TEXT UNIQUE,
            student_face TEXT
        );
        """)

        # ENTRY_LOG 테이블 생성
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS ENTRY_LOG (
            entry_log_id INTEGER PRIMARY KEY AUTOINCREMENT,
            student_id INTEGER,
            entry_time DATETIME DEFAULT CURRENT_TIMESTAMP,
            exit_time DATETIME,
            FOREIGN KEY (student_id) REFERENCES MEMBER(student_id)
        );
        """)

        # SEAT 테이블 생성
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS SEAT (
            seat_id INTEGER PRIMARY KEY,
            status INTEGER DEFAULT 0 -- 예약 여부 (0: 예약 안됨, 1: 예약됨)
        );
        """)

        # SEAT_RESERVATION_LOG 테이블 생성
        cursor.execute("""
        CREATE TABLE IF NOT EXISTS SEAT_RESERVATION_LOG (
            reservation_id INTEGER PRIMARY KEY AUTOINCREMENT,
            student_id INTEGER,
            seat_id INTEGER,
            start_time DATETIME DEFAULT CURRENT_TIMESTAMP,
            end_time DATETIME,
            FOREIGN KEY (student_id) REFERENCES MEMBER(student_id),
            FOREIGN KEY (seat_id) REFERENCES SEAT(seat_id)
        );
        """)

        # 테이블 데이터 초기화 (내용 삭제)
        cursor.execute("DELETE FROM SEAT_RESERVATION_LOG;")
        cursor.execute("DELETE FROM ENTRY_LOG;")
        cursor.execute("DELETE FROM SEAT;")
        cursor.execute("DELETE FROM MEMBER;")

        # MEMBER 테이블에 데이터 삽입
        cursor.execute("""
        INSERT INTO MEMBER (student_id, name, rfid_tag_id, student_face)
        VALUES
            ('20210001', 'Brown', '027812690245', 'brown.jpg'),
            ('20210002', 'Oreo', '729441154526', 'oreo.jpg');
        """)

        # SEAT 테이블에 데이터 삽입
        cursor.execute("""
        INSERT INTO SEAT (seat_id)
        VALUES
            (1), (2), (3), (4);
        """)

        # SEAT 테이블에 데이터 삽입
        cursor.execute("""
        INSERT INTO SEAT_RESERVATION_LOG (student_id, seat_id, start_time)
        VALUES (20210002, 2, CURRENT_TIMESTAMP);
        """)

        # SEAT 테이블에 데이터 삽입
        cursor.execute("""
        UPDATE SEAT
        SET status = 1
        WHERE seat_id = 2;
        """)


        # 변경사항 저장
        conn.commit()
        print("모든 테이블이 성공적으로 생성되었습니다.")
        return 1

    except sqlite3.Error as e:
        print(f"Error: {e}")  # 에러를 로그에 출력
        return 0  # 실패 시 0 반환
    
    finally:
        conn.close()  # conn.close()는 반드시 여기서 호출

def find_face_path(rfid):
    try:
        conn = sqlite3.connect("database.db")
        cursor = conn.cursor()

        query = "SELECT student_face FROM MEMBER WHERE rfid_tag_id = ?;"
        cursor.execute(query, (rfid,))
        result = cursor.fetchone()
        result = my_camera.find_image(result[0])  # Assuming camera.find_image takes a face image path
        return result
    except sqlite3.Error as e:
        print(f"Error: {e}")  # 에러를 로그에 출력
        return None
    finally:
        conn.close()  # conn.close()는 반드시 여기서 호출

def entry_log_insert(student_id):
    try:
        conn = sqlite3.connect("database.db")
        cursor = conn.cursor()

        query = """
        INSERT INTO ENTRY_LOG (student_id)
        VALUES (?);
        """
        cursor.execute(query, (student_id,))
        conn.commit()
        return 1
    except sqlite3.Error as e:
        print(f"Error: {e}")  # 에러를 로그에 출력
        return 0  # 실패 시 0 반환
    finally:
        conn.close()  # conn.close()는 반드시 여기서 호출

def check_reservation(rfid):    
    try:
        conn = sqlite3.connect("database.db")
        cursor = conn.cursor()

        # 1. RFID로 MEMBER 테이블에서 student_id 가져오기
        cursor.execute("SELECT student_id FROM MEMBER WHERE rfid_tag_id = ?", (rfid,))
        result = cursor.fetchone()
        if not result:
            return 0  # RFID가 등록되지 않은 경우 예약 중 아님
        student_id = result[0]

        # 2. SEAT_RESERVATION_LOG에서 현재 예약 상태 확인
        current_time = datetime.now()
        cursor.execute("""
            SELECT 1 
            FROM SEAT_RESERVATION_LOG
            WHERE student_id = ?
            AND (end_time IS NULL OR end_time > ?)
        """, (student_id, current_time))
        reservation = cursor.fetchone()

        # 예약 중이면 1, 아니면 0 반환
        return 1 if reservation else 0

    except sqlite3.Error as e:
        print(f"Error: {e}")  # 에러를 로그에 출력
        return 0  # 실패 시 0 반환
    finally:
        conn.close()  # conn.close()는 반드시 여기서 호출

def reserve_seat(rfid, seat_id):
    rfid = "027812690245"
    print("[DB] Checking if reserve_seat function is loaded")
    try:
        conn = sqlite3.connect("database.db")
        cursor = conn.cursor()

        # 1. RFID로 student_id 찾기
        cursor.execute("""SELECT student_id 
                        FROM MEMBER 
                        WHERE rfid_tag_id = ?;""", ( rfid,))
        student = cursor.fetchone()
        if student is None:
            print(f"해당 RFID {rfid}에 해당하는 학생이 존재하지 않습니다.")
            return 0
        student_id = student[0]

        # 2. SEAT 상태 확인
        cursor.execute("""SELECT status FROM SEAT WHERE seat_id = ?;""", (seat_id,))
        seat_status = cursor.fetchone()

        if seat_status is None or seat_status[0] == 1:
            print("이미 예약된 좌석이거나 존재하지 않는 좌석입니다.")
            return 0

        # 3. SEAT_RESERVATION_LOG에 예약 기록 추가
        cursor.execute("""INSERT INTO SEAT_RESERVATION_LOG (student_id, seat_id, start_time)
                          VALUES (?, ?, CURRENT_TIMESTAMP);""", (student_id, seat_id))

        # 4. SEAT 상태 업데이트
        cursor.execute("""UPDATE SEAT SET status = 1 WHERE seat_id = ?;""", (seat_id,))

        # 변경사항 저장
        conn.commit()
        print(f"좌석 {seat_id} 예약이 완료되었습니다.")
        return 1

    except sqlite3.Error as e:
        print(f"Error: {e}")
        return 0

    finally:
        conn.close()  # conn.close()는 반드시 여기서 호출

def delete_reservation(rfid):
    try:
        conn = sqlite3.connect("database.db")
        cursor = conn.cursor()

        # 1. RFID로 student_id 가져오기
        cursor.execute("SELECT student_id FROM MEMBER WHERE rfid_tag_id = ?", (rfid,))
        result = cursor.fetchone()
        if not result:
            print("해당 RFID에 해당하는 학생이 없습니다.")
            return 0
        student_id = result[0]

        # 2. 현재 활성화된 예약 가져오기
        cursor.execute("""
            SELECT seat_id 
            FROM SEAT_RESERVATION_LOG
            WHERE student_id = ?
              AND (end_time IS NULL OR end_time > ?)
        """, (student_id, datetime.now()))
        reservation = cursor.fetchone()

        if not reservation:
            print("현재 활성화된 예약이 없습니다.")
            return 0
        seat_id = reservation[0]

        # 3. SEAT_RESERVATION_LOG의 end_time 업데이트
        current_time = datetime.now()
        cursor.execute("""
            UPDATE SEAT_RESERVATION_LOG
            SET end_time = ?
            WHERE student_id = ?
              AND seat_id = ?
              AND (end_time IS NULL OR end_time > ?)
        """, (current_time, student_id, seat_id, current_time))

        # 4. SEAT 테이블의 status를 0으로 업데이트
        cursor.execute("""
            UPDATE SEAT
            SET status = 0
            WHERE seat_id = ?
        """, (seat_id,))

        # 변경사항 저장
        conn.commit()
        print(f"예약이 성공적으로 취소되었습니다. 좌석 {seat_id}이 초기화되었습니다.")
        return 1
    except sqlite3.Error as e:
        print(f"Error: {e}")
        return 0
    finally:
        conn.close()  # conn.close()는 반드시 여기서 호출

def get_available_seats():
    print("get_available_seats")
    try:
        conn = sqlite3.connect("database.db")
        cursor = conn.cursor()

        query = """
        SELECT * 
        FROM SEAT
        WHERE status = 0
        """
        cursor.execute(query)
        result = cursor.fetchall()

        # 결과를 seat_list로 변환하고 문자열로 변환
        seat_string = ''.join(str(row[0]) for row in result)
        seat_int = int(seat_string)  # 0
        print(seat_int)
        return seat_int
    except sqlite3.Error as e:
        print(f"Error: {e}")
        return "0"  # 오류 발생 시 "0" 문자열 반환
    finally:
        conn.close()  # conn.close()는 반드시 여기서 호출

def increase_count(seat_id):
    try:
        conn = sqlite3.connect("database.db")
        cursor = conn.cursor()

        # 1. SEAT_RESERVATION_LOG 테이블에서 seat_id와 연결된 student_id 가져오기
        cursor.execute("""
            SELECT student_id 
            FROM SEAT_RESERVATION_LOG
            WHERE seat_id = ?
              AND (end_time IS NULL OR end_time > CURRENT_TIMESTAMP)
        """, (seat_id,))
        result = cursor.fetchone()
        
        if not result:
            return 0  # 해당 seat_id로 예약 중인 사용자가 없을 경우
        
        student_id = result[0]
        
        # 2. count 증가시키기
        cursor.execute("""
            UPDATE SEAT_RESERVATION_LOG
            SET count = count + 1
            WHERE student_id = ?
              AND seat_id = ?
              AND (end_time IS NULL OR end_time > CURRENT_TIMESTAMP)
        """, (student_id, seat_id))
            
        # 3. 증가된 count 값 가져오기
        cursor.execute("""
            SELECT count
            FROM SEAT_RESERVATION_LOG
            WHERE student_id = ?
              AND seat_id = ?
              AND (end_time IS NULL OR end_time > CURRENT_TIMESTAMP)
        """, (student_id, seat_id))
        updated_count = cursor.fetchone()

        # 변경사항 저장
        conn.commit()

        # 4. count 값이 3 이상인 경우 예약 취소
        if updated_count and updated_count[0] >= 3:
            delete_reservation_by_seat(seat_id)
                    
        return 1
    except sqlite3.Error as e:
        print(f"Error: {e}")
        return 0
    finally:
        conn.close()  # conn.close()는 반드시 여기서 호출

def zero_count(rfid):
    try:
        conn = sqlite3.connect("database.db")
        cursor = conn.cursor()

        # 1. RFID로 student_id 가져오기
        cursor.execute("SELECT student_id FROM MEMBER WHERE rfid_tag_id = ?", (rfid,))
        result = cursor.fetchone()
        if not result:
            print("해당 RFID에 해당하는 학생이 없습니다.")
            return 0
        student_id = result[0]

        # 2. count 값 0으로 업데이트
        cursor.execute("""
            UPDATE SEAT_RESERVATION_LOG
            SET count = 0
            WHERE student_id = ?
              AND (end_time IS NULL OR end_time > CURRENT_TIMESTAMP)
        """, (student_id,))

        # 변경사항 저장
        conn.commit()
        return 1
    except sqlite3.Error as e:
        print(f"Error: {e}")
        return 0
    finally:
        conn.close()  # conn.close()는 반드시 여기서 호출

if __name__ == "__main__":
    # 데이터베이스 연결 (파일 없으면 생성)
    make_db()
    #find_face_path("027812690245") # [('brown.jpg',)]
    reserve_seat("027812690245", 3)
