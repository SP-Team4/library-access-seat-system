# SeatGuard: 도서관 출입 및 좌석 관리 시스템

## 개요
**SeatGuard**는 도서관의 부정 출입 및 좌석 오남용 문제를 해결하기 위한 IoT 기반 통합 관리 시스템입니다. RFID, 얼굴 인식, 초음파 센서, 라즈베리파이와 같은 최신 기술을 활용하여 도서관 자원을 자동화하고 효율적으로 관리합니다.

---

## 주요 기능
- **이중 인증**: RFID와 얼굴 인식을 결합한 안전한 사용자 인증 제공.
- **좌석 예약**: RFID 카드와 LED 표시등을 활용한 실시간 좌석 예약 시스템.
- **좌석 점유 모니터링**: 초음파 센서를 통해 좌석 점유 상태를 실시간으로 감지.
- **중앙 집중식 관리**: 중앙 서버를 통해 데이터 동기화, 사용자 로그 및 좌석 상태 관리.
- **효율적인 자원 관리**: 서버 중심 제어를 통해 센서 작동과 시스템 자원 최적화.

---

## 시스템 구조
### 구성 요소
1. **하드웨어**:
   - 라즈베리파이 4
   - RFID 리더기 (MFRC522)
   - PIR 모션 센서
   - 초음파 센서 (HC-SR04)
   - 서보 모터
   - I2C LCD 디스플레이
   - 버튼 및 LED

2. **소프트웨어**:
   - Python: 데이터베이스 및 하드웨어 연동
   - C: 고성능 서버 운영
   - SQLite3: 데이터 영구 저장
   - OpenCV 및 ORB: 이미지 처리

---

## 워크플로우
### 출입 관리
1. 사용자가 RFID와 얼굴 인식을 통해 인증합니다.
2. PIR 센서를 통해 입장 또는 퇴장 방향을 감지합니다.
3. 인증 결과에 따라 서보 모터로 게이트를 제어합니다.

### 좌석 예약
1. LED로 사용 가능한 좌석을 표시합니다.
2. RFID 인증 후 버튼을 눌러 좌석을 선택합니다.
3. 예약 데이터가 서버와 동기화됩니다.

### 좌석 모니터링
1. 초음파 센서를 통해 좌석 점유 상태를 실시간 모니터링합니다.
2. 부정 사용 또는 장기 미사용 시 서버에 알림을 보냅니다.

---

## 팀원
<table>
  <tr>
    <td align="center">
      <img src="https://avatars.githubusercontent.com/yesolz" width="70" height="70" /><br />
      <a href="https://github.com/yesolz">최예솔</a>
    </td>
    <td align="center">
      <img src="https://avatars.githubusercontent.com/bbanghe" width="70" height="70" /><br />
      <a href="https://github.com/HANTAEDONG">박세연</a>
    </td>
    <td align="center">
      <img src="https://avatars.githubusercontent.com/tinon1004" width="70" height="70" /><br />
      <a href="https://github.com/JNL-2002">조나애</a>
    </td>
    <td align="center">
      <img src="https://avatars.githubusercontent.com/bumjuni" width="70" height="70" /><br />
      <a href="https://github.com/OfficialJOLO">장지윤</a>
    </td>
  </tr>
</table>

---
