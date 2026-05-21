# PassKeeper

Local desktop password manager built with C++20, Qt Quick/QML, PostgreSQL/libpq and OpenSSL.

## Architecture

- `ui/` - QML-facing controller.
- `qml/` - Qt Quick/QML screens and controls.
- `core/` - runtime application context.
- `crypto/` - PBKDF2-HMAC-SHA256, AES-256-GCM and secure password generation.
- `database/` - direct PostgreSQL/libpq connection and schema bootstrapping.
- `models/` - DTO-style application models.
- `services/` - authentication, categories, entries, clipboard and import/export.
- `utils/` - small helpers.

## Cryptographic Scheme

- Master password is never stored.
- On first launch the app creates a 128-bit random salt and derives 64 bytes with PBKDF2-HMAC-SHA256.
- The first 32 bytes are the AES-256-GCM vault key kept only in memory after unlock.
- The second 32 bytes are hashed as a verifier and stored in `app_settings`.
- Every encrypted field has its own 96-bit GCM nonce and 128-bit authentication tag.
- Entry field AAD binds ciphertext to `entry_id` and field name.
- Export files are JSON envelopes encrypted with AES-256-GCM using a key derived from the export password.

For production hardening, prefer Argon2id through libsodium or OpenSSL provider support when available,
raise KDF parameters after benchmarking on target machines, and consider encrypting titles too if metadata
confidentiality is required.

## PostgreSQL Setup

Create a local database:

```bash
createdb passkeeper
```

The app connects to PostgreSQL directly through `libpq`; no Qt `QPSQL` SQL plugin is required.

The app reads connection settings from environment variables:

```bash
export PASSKEEPER_DB_HOST=localhost
export PASSKEEPER_DB_PORT=5432
export PASSKEEPER_DB_NAME=passkeeper
export PASSKEEPER_DB_USER=postgres
export PASSKEEPER_DB_PASSWORD=''
```

Or create a local `.env` file near the executable or in the project root:

```env
PASSKEEPER_DB_HOST=localhost
PASSKEEPER_DB_PORT=5432
PASSKEEPER_DB_NAME=passkeeper
PASSKEEPER_DB_USER=postgres
PASSKEEPER_DB_PASSWORD=your_postgres_password
```

## Build

```bash
cmake -S . -B build
cmake --build build
./build/passkeeper
```

On Apple Silicon with Homebrew Qt and keg-only `libpq`, use:

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt \
  -DPostgreSQL_ROOT=/opt/homebrew/opt/libpq
cmake --build build
./build/passkeeper
```

Dependencies:

- Qt 6 or Qt 5 with `Quick` and `QuickControls2`
- PostgreSQL client library `libpq`
- OpenSSL
- CMake 3.20+

## SQL Schema

The canonical SQL file is [`schema/schema.sql`](schema/schema.sql).
