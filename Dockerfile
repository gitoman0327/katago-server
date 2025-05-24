FROM python:3.8

WORKDIR /app

# 필요한 시스템 라이브러리 설치
RUN apt-get update && apt-get install -y \
    libzip4 \
    libzip-dev \
    zlib1g-dev \
    libboost-all-dev \
    wget \
    unzip \
    && rm -rf /var/lib/apt/lists/*

# 소스 코드 복사
COPY . .

# 필요한 Python 패키지 설치
RUN pip install -r requirements.txt

# 포트 설정
EXPOSE 8080

# 시작 명령어
CMD ["gunicorn", "app:app", "--bind", "0.0.0.0:8080", "--workers", "1", "--threads", "8"]