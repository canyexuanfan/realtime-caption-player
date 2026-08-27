PRAGMA foreign_keys = ON;

CREATE TABLE schema_meta (
    version INTEGER NOT NULL
);

CREATE TABLE media (
    id INTEGER PRIMARY KEY,
    canonical_url TEXT NOT NULL,
    file_size INTEGER,
    modified_time_ms INTEGER,
    duration_ms INTEGER,
    last_position_ms INTEGER NOT NULL DEFAULT 0,
    last_played_at_ms INTEGER,
    selected_audio_ff_index INTEGER,
    created_at_ms INTEGER NOT NULL,
    updated_at_ms INTEGER NOT NULL,
    UNIQUE(canonical_url, file_size, modified_time_ms)
);

CREATE TABLE caption_session (
    id TEXT PRIMARY KEY,
    media_id INTEGER NOT NULL REFERENCES media(id) ON DELETE CASCADE,
    audio_ff_index INTEGER NOT NULL,
    model_bundle_id TEXT NOT NULL,
    model_bundle_version TEXT NOT NULL,
    language TEXT NOT NULL,
    profile TEXT NOT NULL,
    status TEXT NOT NULL,
    covered_until_ms INTEGER NOT NULL DEFAULT 0,
    created_at_ms INTEGER NOT NULL,
    updated_at_ms INTEGER NOT NULL
);

CREATE TABLE caption_segment (
    session_id TEXT NOT NULL REFERENCES caption_session(id) ON DELETE CASCADE,
    segment_id TEXT NOT NULL,
    revision INTEGER NOT NULL,
    start_ms INTEGER NOT NULL,
    end_ms INTEGER NOT NULL,
    text TEXT NOT NULL,
    language TEXT,
    source TEXT NOT NULL,
    created_at_ms INTEGER NOT NULL,
    PRIMARY KEY(session_id, segment_id),
    CHECK(start_ms >= 0),
    CHECK(end_ms > start_ms),
    CHECK(revision >= 1)
);

CREATE INDEX idx_caption_segment_time
ON caption_segment(session_id, start_ms, end_ms);

CREATE TABLE playlist (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    created_at_ms INTEGER NOT NULL,
    updated_at_ms INTEGER NOT NULL
);

CREATE TABLE playlist_item (
    playlist_id TEXT NOT NULL REFERENCES playlist(id) ON DELETE CASCADE,
    position INTEGER NOT NULL,
    url TEXT NOT NULL,
    display_name TEXT,
    PRIMARY KEY(playlist_id, position)
);

INSERT INTO schema_meta (version) VALUES (1);
