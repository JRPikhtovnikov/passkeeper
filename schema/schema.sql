CREATE TABLE IF NOT EXISTS app_settings (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS categories (
    id BIGSERIAL PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS password_entries (
    id BIGSERIAL PRIMARY KEY,
    category_id BIGINT REFERENCES categories(id) ON DELETE SET NULL,
    title TEXT NOT NULL,

    username_nonce BYTEA NOT NULL,
    username_ciphertext BYTEA NOT NULL,
    username_tag BYTEA NOT NULL,

    password_nonce BYTEA NOT NULL,
    password_ciphertext BYTEA NOT NULL,
    password_tag BYTEA NOT NULL,

    url_nonce BYTEA NOT NULL,
    url_ciphertext BYTEA NOT NULL,
    url_tag BYTEA NOT NULL,

    notes_nonce BYTEA NOT NULL,
    notes_ciphertext BYTEA NOT NULL,
    notes_tag BYTEA NOT NULL,

    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_password_entries_title ON password_entries USING btree (lower(title));
CREATE INDEX IF NOT EXISTS idx_password_entries_category_id ON password_entries(category_id);

