.PHONY: all run debug clean format check test

all:
	cargo build --release

run:
	cargo run --release

debug:
	cargo run

clean:
	cargo clean
	@rm -rf pkg target*

format:
	cargo fmt

check:
	cargo clippy -- -W clippy::all -W clippy::correctness -W clippy::suspicious -W clippy::complexity -W clippy::perf -W clippy::style -W clippy::pedantic -W clippy::panic -A clippy::doc_markdown -A clippy::wildcard_imports -A clippy::module_name_repetitions -A clippy::missing_errors_doc -A clippy::missing_panics_doc -D warnings

test:
	cargo test -- --nocapture
