-- TODO App Schema
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
