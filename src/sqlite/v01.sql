CREATE TABLE cimple_repositories (
	id INTEGER PRIMARY KEY,
	url TEXT NOT NULL
) STRICT;

CREATE UNIQUE INDEX cimple_repositories_url_index ON cimple_repositories(url);

CREATE TABLE cimple_run_status (
	id INTEGER PRIMARY KEY,
	label TEXT NOT NULL
) STRICT;

CREATE UNIQUE INDEX cimple_run_status_label_index ON cimple_run_status(label);

INSERT INTO cimple_run_status(id, label) VALUES (0, 'created');
INSERT INTO cimple_run_status(id, label) VALUES (1, 'finished');

CREATE TABLE cimple_runs (
	id INTEGER PRIMARY KEY,
	status INTEGER NOT NULL,
	result INTEGER NOT NULL,
	output BLOB NOT NULL,
	repo_id INTEGER NOT NULL,
	FOREIGN KEY (status) REFERENCES cimple_run_status(id),
	FOREIGN KEY (repo_id) REFERENCES cimple_repositories(id)
		ON DELETE CASCADE ON UPDATE CASCADE
) STRICT;

CREATE INDEX cimple_runs_status_index ON cimple_runs(status);
CREATE INDEX cimple_runs_repo_id_index ON cimple_runs(repo_id);
