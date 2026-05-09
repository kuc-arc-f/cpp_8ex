# http_11pg

 Version: 0.9.1

 date    : 2026/05/08

 update :

***

C++ Linux , postgreSQL + React todo API

* LLVM CLang use
* postgreSQL DB
* node 22

***
### related

* https://github.com/nlohmann/json

* https://github.com/yhirose/cpp-httplib/blob/master/httplib.h

* dotenv-cpp
* https://github.com/laserpants/dotenv-cpp

***

### setup

* LIB

***
* httplib.h
```
wget https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.43.2.tar.gz
tar xvf v0.43.2.tar.gz
cp cpp-httplib-0.43.2/httplib.h  ./include
rm -fr cpp-httplib-0.43.2/
```

***
* dotenv-cpp

```
git clone https://github.com/laserpants/dotenv-cpp.git
mkdir -p include
cp -rn <path-to-this-repo>/dotenv-cpp/include/ .
```

***

```
sudo apt-get update
sudo apt-get install libpq-dev
sudo apt install nlohmann-json3-dev
```
***
* tree
```
$ tree include/
include/
├── httplib.h
├── laserpants
│   └── dotenv
│       └── dotenv.h
└── my_db.hpp
```
***
### env
```
export DATABASE_URL="host=localhost port=5432 dbname=mydb user=root password=admin"
```
***
### .env
```
VALID_LOGIN=true
USER_NAME=user123@example.com
PASSWORD=123
```


***
* table: schema.sql
***
* build

```
make all
```

***
### front-build

```
npm i
npm run build
```
***
* start
* localhost:8000 start

```
./server
```
***
