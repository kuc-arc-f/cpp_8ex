# http_6pg

 Version: 0.9.1

 date    : 2026/05/08

 update :

***

C++ Linux , postgreSQL + todo API  example

* LLVM CLang use
* postgreSQL DB

***
### related

* https://github.com/nlohmann/json

* https://github.com/yhirose/cpp-httplib/blob/master/httplib.h

***

### setup

* LIB

***
* httplib.h
```
wget https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.43.2.tar.gz
tar xvf v0.43.2.tar.gz
cp cpp-httplib-0.43.2/httplib.h  .
rm -fr cpp-httplib-0.43.2/
```
***

```
sudo apt-get update
sudo apt-get install libpq-dev
sudo apt install nlohmann-json3-dev
```

***
### env
```
export DATABASE_URL="host=localhost port=5432 dbname=mydb user=root password=admin"
```

***
* table: schema.sql
```sql
CREATE TABLE IF NOT EXISTS todos (
    id          SERIAL PRIMARY KEY,
    title       VARCHAR(255) NOT NULL,
    description TEXT,
    due_date    DATE,
    priority    SMALLINT DEFAULT 0,   -- 0:low 1:medium 2:high
    done        BOOLEAN NOT NULL DEFAULT FALSE,
    created_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at  TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
```

***
* build

```
make all
```
***
* start
* localhost:8000 start

```
./server
```

***
### test-data

* add
```
curl -X POST http://localhost:8000/todos \
  -H "Content-Type: application/json" \
  -d '{
    "title": "買い物",
    "description": "牛乳とパン",
    "due_date": "2026-04-01",
    "priority": 1,
    "done": false
  }'

```

* List

```
curl http://localhost:8000/todos
```

* DELETE
```
curl -X DELETE http://localhost:8000/todos/1
```

***
