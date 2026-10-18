-- Phase 3 migration for databases created before this change.
-- (A fresh `docker compose up` already gets this from sql/01_init_schema.sql;
--  files in scripts/ are NOT auto-run, so apply this once by hand:)
--
--   docker exec -i allotmate_postgres psql -U postgres -d allotmate < scripts/migrate_phase3.sql
--
-- RUN_LOG.blocking_pairs_found becomes nullable: NULL means "the stability
-- check was not run", so a run is never reported as having 0 blocking pairs
-- unless that was actually verified.

ALTER TABLE RUN_LOG ALTER COLUMN blocking_pairs_found DROP DEFAULT;
ALTER TABLE RUN_LOG ALTER COLUMN blocking_pairs_found DROP NOT NULL;
