CREATE TABLE cimple_repos (
	id INTEGER PRIMARY KEY,
	url TEXT NOT NULL
) STRICT;

CREATE UNIQUE INDEX cimple_repos_index_url ON cimple_repos(url);

CREATE TABLE cimple_run_status (
	id INTEGER PRIMARY KEY,
	label TEXT NOT NULL
) STRICT;

CREATE UNIQUE INDEX cimple_run_status_index_label ON cimple_run_status(label);

INSERT INTO cimple_run_status(id, label) VALUES (1, 'created');
INSERT INTO cimple_run_status(id, label) VALUES (2, 'finished');

CREATE TABLE cimple_runs (
	id INTEGER PRIMARY KEY,
	status INTEGER NOT NULL,
	exit_code INTEGER NOT NULL,
	output BLOB NOT NULL,
	repo_id INTEGER NOT NULL,
	repo_rev TEXT NOT NULL,
	FOREIGN KEY (status) REFERENCES cimple_run_status(id),
	FOREIGN KEY (repo_id) REFERENCES cimple_repos(id)
		ON DELETE CASCADE ON UPDATE CASCADE
) STRICT;

CREATE INDEX cimple_runs_index_status ON cimple_runs(status);
CREATE INDEX cimple_runs_index_repo_id ON cimple_runs(repo_id);

CREATE VIEW cimple_runs_view(id, status, exit_code, output, repo_url, repo_rev)  AS
	SELECT run.id, status.label, run.exit_code, run.output, repo.url, run.repo_rev FROM cimple_runs AS run
		INNER JOIN cimple_run_status as status ON run.status = status.id
		INNER JOIN cimple_repos as repo ON run.repo_id = repo.id;
