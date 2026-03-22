# Contributing

## Terraform Applies

`terraform apply` is handled by the lead dev for now. Open a PR with your `.tf` changes, get it reviewed and merged, then ping in the group chat so the apply doesn't get missed. Don't apply from your own machine unless you've been explicitly handed the keys.

This keeps things simple for a team of 3. If the team grows, we'll look at Terraform Cloud or GitHub Actions OIDC.

## AWS Infrastructure Changes

All infrastructure changes must go through Terraform — never click-ops in the console.

1. Edit `.tf` files
2. `terraform plan` — review the diff
3. PR review
4. `terraform apply` after merge to main

## Firmware Changes

Test locally with Serial Monitor before committing. Note in the PR description what hardware was tested with.

## Lambda Changes

After merging Lambda code changes, run `terraform apply` to redeploy (Terraform detects the source hash change automatically via `archive_file`).

## Adding New Sensors

### Car Unit
1. Add wiring to `docs/HARDWARE.md`
2. Add library to prerequisites in `docs/HARDWARE.md`
3. Add fields to TEL message protocol in firmware and `docs/ARCHITECTURE.md`
4. Update `lambda/telemetry-ingest` to store new fields
5. No DynamoDB schema change needed (DynamoDB is schemaless for non-key attributes)

### Base Station
1. Add wiring to `docs/HARDWARE.md`
2. Create a new POST endpoint if the sensor type doesn't fit `/weather`
3. Add Lambda, API Gateway route, and DynamoDB table in Terraform

## Phase Roadmap

See `TODO.md` for current priorities. Open a GitHub issue to discuss any Phase 2+ work before starting implementation.
