# Current evidence

- `verification.json`: point-in-time source/index/body-hash audit.
- `sources.json`: apparent source-file counts/bytes by component.
- `benchmarks.json`: repeated warm-engine measurements for configured queries.

These records are read on demand, not loaded at startup. They do not prove future
freshness, compiler semantics or device behavior. Historical prototype reports
were removed from the active tree after preserving recoverable Git history.

- [Package acceptance](package-check.json): source-free restoration and CLI checks (dated 2.1.1 record).

- [Final tool review](review.json): patch scope, fixtures and review limits (dated 2.1.1 record).

- [Fresh index-only clone](fresh-clone.json): reusable-command acceptance, isolation and query guards.
