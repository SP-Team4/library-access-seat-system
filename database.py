import sqlite3
import camera

def make_db():
    conn = sqlite3.connect("database.db")
    cursor = conn.cursor()

    # MEMBER 테이블 생성
    cursor.execute("""
        -- MEMBER 테이블
        CREATE TABLE MEMBER (
            student_id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT,
            rfid_tag_id TEXT UNIQUE,
            student_face TEXT,
        );

        -- ENTRY_LOG 테이블
        CREATE TABLE ENTRY_LOG (
            entry_log_id INTEGER PRIMARY KEY AUTOINCREMENT,
            student_id INTEGER,
            entry_time DATETIME DEFAULT CURRENT_TIMESTAMP,
            exit_time DATETIME,
            FOREIGN KEY (student_id) REFERENCES MEMBER(student_id)
        );
r
        -- SEAT 테이블
        CREATE TABLE SEAT (
            seat_id INTEGER PRIMARY KEY,
            is_reserved INTEGER DEFAULT 0
        );

        -- SEAT_RESERVATION_LOG 테이블
        CREATE TABLE SEAT_RESERVATION_LOG (
            reservation_id INTEGER PRIMARY KEY AUTOINCREMENT,
            student_id INTEGER,
            seat_id INTEGER,
            status INTEGER, 
            start_time DATETIME DEFAULT CURRENT_TIMESTAMP,
            end_time DATETIME,
            is_force_cancelled INTEGER DEFAULT 0,
            FOREIGN KEY (student_id) REFERENCES MEMBER(student_id),
            FOREIGN KEY (seat_id) REFERENCES SEAT(seat_id)
        );

        -- SEAT_STATUS 테이블
        CREATE TABLE SEAT_STATUS (
            seat_id INTEGER,
            status INTEGER,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            count INTEGER DEFAULT 0,
            FOREIGN KEY (seat_id) REFERENCES SEAT(seat_id)
        );
    """)
    
    #insert 예시
    cursor.execute("""
        INSERT INTO MEMBER (student_id, name, rfid_tag_id, student_face)
        VALUES
            ('20210001', 'Brown', '027812690245','brown.jpg'),
            ('20210002', 'Oreo', '729441154526', 'oreo.jpg');

        -- ENTRY_LOG 테이블 데이터 삽입 => Entry가 발생할 때마다 insert한다. 

        -- SEAT 테이블 데이터 삽입 => Default
        INSERT INTO SEAT (seat_id , is_reserved)
        VALUES
            (1, 0),
            (2, 0),
            (3, 0), 
            (4, 0);
    """)

    # 변경사항 저장
    conn.commit()
    print("모든 테이블이 성공적으로 생성되었습니다.")
    # 연결 종료
    conn.close()


def find_face_path(rfid):
    '''
    conn = sqlite3.connect("database.db")
    cursor = conn.cursor()

    query = """
        SELECT student_face 
        FROM MEMBER 
        WHERE rfid_tag_id = ?;"""
    cursor.execute(query, (rfid,))
    result = cursor.fetchone()
    print(face)

    # 연결 종료
    conn.close()
    '''
    result = camera.find_image("brown.jpg")
    print(result)
    return(result)

def entry_log_insert(student_id):
    query = """
    INSERT INTO ENTRY_LOG (student_id )
    VALUES (?);
    """
    
    cursor.execute(query, (student_id))
    conn.commit()

def check_reservation(rfid):
    query = """
        select status
        from SEAT_RESERVATION_LOG
        where student_id = (select student_id from member  where rfid = ? );
    """
    cursor.execute(query, (rfid,))
    result = cursor.fetchone()

def delete_reservation(rfid):
    query1 = """
        UPDATE SEAT_STATUS
        SET status = 0
        WHERE seat_id = (SELECT seat_id FROM SEAT_RESERVATION_LOG 
                        WHERE student_id = (SELECT student_id FROM MEMBER WHERE rfid_tag_id = ?));
    """

    query2 = """
        UPDATE SEAT_RESERVATION_LOG
        SET status = 0
        WHERE student_id = (SELECT student_id FROM MEMBER WHERE rfid_tag_id = ?);
    """

    cursor.execute(query1, (rfid,))
    cursor.execute(query2, (rfid,))
    conn.commit()
    result = cursor.fetchone()

if __name__ == "__main__":
    # 데이터베이스 연결 (파일 없으면 생성)
    find_face_path("123456") # [('brown.jpg',)]

