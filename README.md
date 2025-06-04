# 📡 IoT 기반 스마트 사무실 관리 시스템

![Platform](https://img.shields.io/badge/Platform-Arduino%20%7C%20Raspberry%20Pi-blue?style=flat-square)
![Language](https://img.shields.io/badge/Language-C-green?style=flat-square)
![Database](https://img.shields.io/badge/Database-MariaDB-blue?style=flat-square)

## 📘 프로젝트 개요

이 프로젝트는 Arduino와 Raspberry Pi를 활용하여 사무실의 환경을 모니터링하고 제어하는 스마트 사무실 관리 시스템입니다. 센서 데이터를 수집하고, 이를 실시간으로 데이터베이스에 저장하여 효율적인 사무실 환경 관리를 목표로 합니다.

## 🛠 주요 기능

- **환경 모니터링**: 온도, 습도, 조도 등의 센서 데이터를 실시간으로 수집
- **데이터 저장**: 수집된 데이터를 MariaDB에 저장하여 이력 관리
- **제어 기능**: 수집된 데이터를 기반으로 사무실의 조명 및 환기 시스템 제어

## 🧰 사용 기술

- **하드웨어**:
  - Arduino Uno
  - Raspberry Pi 4
  - DHT11 (온습도 센서)
  - LDR (조도 센서)
- **소프트웨어**:
  - Arduino IDE
  - Python 3
  - MariaDB
- **통신**:
  - 시리얼 통신 (Arduino ↔ Raspberry Pi)
  - Wi-Fi (Raspberry Pi ↔ 서버)

## 📦 설치 및 실행 방법

### 1. Arduino 설정

- Arduino IDE를 설치하고, 필요한 라이브러리를 추가합니다.
- `arduino/` 디렉토리 내의 `.ino` 파일을 Arduino 보드에 업로드합니다.

<!-- ### 2. Raspberry Pi 설정

- Raspberry Pi에 Python 3과 필요한 라이브러리를 설치합니다:
  ```bash
  sudo apt-get update
  sudo apt-get install python3-pip
  pip3 install pymysql
  ```
- `raspberry_pi/` 디렉토리 내의 Python 스크립트를 실행하여 시리얼 통신을 시작합니다. -->

### 2. MariaDB 설정

- MariaDB를 설치하고, 데이터베이스 및 테이블을 생성합니다:
  ```sql
  CREATE DATABASE office_db;
  USE office_db;
  CREATE TABLE sensor_data (
      id INT AUTO_INCREMENT PRIMARY KEY,
      temperature FLOAT,
      humidity FLOAT,
      light_intensity FLOAT,
      timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
  );
  ```
- Raspberry Pi의 Python 스크립트에서 데이터베이스 연결 정보를 설정합니다.

## 📂 프로젝트 구조

```
IoT_OFFICE_PROJECT/
├── arduino/
│   └── sensor_readings.ino
├── database/
│   └── setup.sql
├── README.md
```

## 📈 시스템 아키텍처

```mermaid
graph LR
A[Arduino] -- 시리얼 통신 --> B[Raspberry Pi]
B -- Wi-Fi --> C[MariaDB]
```

## 📌 참고 사항

- 시스템의 확장성을 고려하여 모듈화된 구조로 설계되었습니다.
- 향후 웹 대시보드 및 모바일 앱과의 연동을 통해 사용자 인터페이스를 개선 가능 합니다.


## 📅 프로젝트 진행 기간

**2023년 11월 20일 ~ 2023년 11월 28일**


## 📄 라이선스

이 프로젝트는 MIT 라이선스를 따릅니다. 자세한 내용은 `LICENSE` 파일을 참고하세요.

---

<!-- # IoT_OFFICE_PROJECT  
IoT Control OFFICE 프로젝트  

## 설명  
WIFI, BlueTooth를 이용하여 RFID에 등록 여부를 확인 하여 반응 하는 스마트 오피스를 구현하는 프로젝트  

===========================================================

## Project 이미지  
<b><미리 등록한 데이터 베이스></b> 
![img1](presentation_data/데이터베이스.PNG)  
<b><등록된 사용자의 경우 반응></b>  
<img src="presentation_data/등록된경우.jpg" width="300" height="200">  
<b><등록 되지 않은 사용자의 경우 반응></b>  
<img src="presentation_data/미등록된경우.jpg" width="300" heigth="200">  
 

## 작동 시연 영상  
![gif1](presentation_data/GIFMaker_me.gif)  -->
