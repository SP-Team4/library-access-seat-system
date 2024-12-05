import sqlite3

def create_tables(cursor):

    # MEMBER 테이블 생성
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS MEMBER (
            student_id INTEGER PRIMARY KEY , -- 고유 ID
            name TEXT NOT NULL, -- 이름
            rfid_tag_id TEXT, -- RFID 태그 (중복 방지)
            is_active INTEGER , -- 활성화 여부
            student_face TEXT -- 파일 이름 (경로는 고정된 디렉토리로 관리)
        );
    """)
    
    #insert 예시
    cursor.execute("""
        INSERT INTO MEMBER (student_id, name, rfid_tag_id, is_active, student_face) VALUES
        ('20210001', 'Brown', '027812690245', 0, 'brown.jpg'),
        ('20210002', 'Oreo', '729441154526', 0, 'oreo.jpg');
    """)

    # 변경사항 저장
    conn.commit()
    print("모든 테이블이 성공적으로 생성되었습니다.")

def find_face_path(cursor):
    cursor.execute("""
    select student_face 
    from member
    where student_id = 20210001;
    """)
    face = cursor.fetchall()
    print(face)

    # 연결 종료
    conn.close()

    return(face)

if __name__ == "__main__":
    # 데이터베이스 연결 (파일 없으면 생성)
    conn = sqlite3.connect("database.db")
    cursor = conn.cursor()
    create_tables(cursor)
    find_face_path(cursor)

