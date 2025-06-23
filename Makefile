.PHONY: all cloud evidence run debug clean format check test

all:
	$(error You must specify one of the following targets: cloud evidence run debug clean format check test)

cloud:
	cargo build --release --package civicalert-cloud

evidence:
	cargo build --release --package civicalert-evidence

run:
	cargo run --release --package civicalert-cloud

debug:
	cargo run --package civicalert-cloud

clean:
	cargo clean
	@rm -rf pkg target*

format:
	cargo fmt

check:
	cargo clippy -- -W clippy::all -W clippy::correctness -W clippy::suspicious -W clippy::complexity -W clippy::perf -W clippy::style -W clippy::pedantic -W clippy::panic -A clippy::doc_markdown -A clippy::wildcard_imports -A clippy::module_name_repetitions -A clippy::missing_errors_doc -A clippy::missing_panics_doc -D warnings

test:
	cargo test -- --nocapture
