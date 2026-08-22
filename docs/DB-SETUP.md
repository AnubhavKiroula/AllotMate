# AllotMate — Database Setup & Team Access Guide

This guide explains how to spin up the local PostgreSQL database using Docker Compose, connect pgAdmin for visual inspection, configure `.env`, and verify your setup.

---

## 1. Prerequisites

- **Docker Desktop**: Ensure Docker Desktop is installed and running on your machine.
- **pgAdmin 4 (Optional GUI)**: Download and install [pgAdmin 4](https://www.pgadmin.org/) if you want a visual GUI for table inspection and running queries. Note that pgAdmin runs natively on your machine and points to the containerized database.

---

## 2. Environment Configuration (`.env`)

Copy the template `.env.example` to `.env` in the root directory:

```bash
cp .env.example .env
```

Ensure your `.env` file contains the following default credentials:

```ini
DB_HOST=localhost
DB_PORT=5432
DB_NAME=allotmate
DB_USER=postgres
DB_PASSWORD=postgres
```

> [!IMPORTANT]
> Never commit your `.env` file to Git. It is already added to `.gitignore`.

---

## 3. Starting the Database with Docker Compose

Start the PostgreSQL service in the background:

```bash
docker compose up -d
```

### What happens automatically:
1. Postgres 16 starts and binds to `localhost:5432`.
2. Initial schema migrations in [`sql/01_init_schema.sql`](file:///c:/Users/Anubhav%20Kiroula/OneDrive/Documents/PBL/AllotMate/sql/01_init_schema.sql) and seed data in [`sql/02_seed_data.sql`](file:///c:/Users/Anubhav%20Kiroula/OneDrive/Documents/PBL/AllotMate/sql/02_seed_data.sql) are automatically executed by Postgres on the initial container run.
3. Data is persisted in a named volume (`pgdata`), surviving container restarts.

### Useful Docker Commands:
- **Check container status:** `docker compose ps`
- **View Postgres logs:** `docker compose logs -f postgres`
- **Stop Postgres:** `docker compose down`
- **Reset database completely (wipe volume & re-seed):**
  ```bash
  docker compose down -v
  docker compose up -d
  ```

---

## 4. Connecting via pgAdmin (Team DB Access)

Any team member can inspect the database visually using pgAdmin 4:

1. Open **pgAdmin 4**.
2. Right-click **Servers** > **Register** > **Server...**.
3. In the **General** tab:
   - **Name**: `AllotMate Local`
4. In the **Connection** tab:
   - **Host name / address**: `localhost`
   - **Port**: `5432`
   - **Maintenance database**: `allotmate`
   - **Username**: `postgres` (or your value from `.env`)
   - **Password**: `postgres` (or your value from `.env`)
   - Check **Save Password**.
5. Click **Save**.
6. Expand **Databases** > **allotmate** > **Schemas** > **public** > **Tables** to view:
   - `STUDENT`
   - `ELECTIVE`
   - `PREFERENCE`
   - `ALLOTMENT`
   - `WAITLIST`
   - `COORDINATOR`
   - `RUN_LOG`

---

## 5. Verifying Connection from C++

Run the application executable after compiling:

```bash
# Windows
.\build\Debug\AllotMate.exe

# Linux / macOS
./build/AllotMate
```

Expected output:
```text
========================================
   AllotMate - DB Layer Connection Test  
========================================
[1/4] Initializing PostgreSQL connection from .env...
 -> Successfully connected to PostgreSQL server.
[2/4] Querying database version and table counts...
 -> DB Server Version: PostgreSQL 16.x ...
 -> Current Records in DB:
    - Students:    100
    - Electives:   6
    - Preferences: 400
[3/4] Performing Phase 1 Student Insert & Read Verification Cycle...
 -> Inserted 1 test student row.
 -> Read back test student successfully:
    ...
 -> Cleaned up test record.
[4/4] Phase 1 Database Connection and CRUD verification completed successfully.
```
