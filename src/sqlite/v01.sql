CREATE TABLE cimple_repositories (
	id INTEGER PRIMARY KEY,
	url TEXT NOT NULL
) STRICT;

CREATE UNIQUE INDEX cimple_repositories_url_index ON cimple_repositories(url);

CREATE TABLE cimple_runs (
	id INTEGER PRIMARY KEY,
	result INTEGER NOT NULL,
	output BLOB NOT NULL,
	repo_id INTEGER NOT NULL,
	FOREIGN KEY (repo_id) REFERENCES cimple_repositories(id)
		ON DELETE CASCADE ON UPDATE CASCADE
) STRICT;

CREATE INDEX cimple_runs_repo_id_index ON cimple_runs(repo_id);
