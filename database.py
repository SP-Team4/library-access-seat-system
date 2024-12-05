import sqlite3

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
        INSERT INTO MEMBER (student_id, name, rfid_tag_id, is_active, student_face)
        VALUES
            ('20210001', 'Brown', '027812690245', 0, 'brown.jpg'),
            ('20210002', 'Oreo', '729441154526', 0, 'oreo.jpg');

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

def find_face_path(rfid):
    conn = sqlite3.connect("database.db")
    cursor = conn.cursor()

    query = """
        SELECT student_face 
        FROM MEMBER 
        WHERE rfid_tag_id = ?;"""
    cursor.execute(query, (rfid,))
    result = cursor.fetchone()print(face)

    # 연결 종료
    conn.close()

    return(face)

if __name__ == "__main__":
    # 데이터베이스 연결 (파일 없으면 생성)
    create_tables(cursor)
    find_face_path(cursor) # [('brown.jpg',)]

