"""Tests for .github/workflows/build.yml.

These are static checks: parse the workflow YAML and assert invariants
that the runtime interpolation `${{ matrix.script }}` cannot enforce.
A failure here means a CI job will silently try to run a missing script
and waste minutes reproducing it on the runner.

Run with:
    pip install -r requirements-dev.txt
    python -m unittest tests.test_build_workflow -v
"""

import os
import re
import unittest

try:
    import yaml
except ImportError:  # pragma: no cover - explicit user-facing failure
    raise SystemExit(
        "PyYAML is required for these tests.\n"
        "Install with: pip install -r requirements-dev.txt"
    )


REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir))
WORKFLOW_PATH = os.path.join(REPO_ROOT, ".github", "workflows", "build.yml")


def _load_workflow():
    # PyYAML 1.1 parses the bare key "on:" as the boolean True (because
    # YAML 1.1 treats "on"/"off"/"yes"/"no" as bools). GitHub Actions
    # workflows use YAML 1.2 semantics. We rewrite the key so SafeLoader
    # round-trips the workflow the way GitHub will see it.
    with open(WORKFLOW_PATH, encoding="utf-8") as f:
        text = f.read()
    # Only rewrite a top-level "on:" key (anchored, line-start). Leave
    # nested "on:" alone -- those are uncommon and would change meaning.
    text = re.sub(r"^on:", '"on":', text, count=1, flags=re.MULTILINE)
    return yaml.safe_load(text)


def _on_key(workflow):
    """Return the parsed value of the workflow's 'on' trigger spec.

    Handles both YAML 1.1 (where 'on' may be parsed as True) and
    YAML 1.2 (where it is a string).
    """
    if "on" in workflow:
        return workflow["on"]
    if True in workflow:
        return workflow[True]
    return None


# Map of job_id -> (matrix_key, script_path_template).
# Each template is the on-disk path that the workflow's run-step will
# invoke, resolved against REPO_ROOT.
JOB_SCRIPT_PATHS = {
    "linux-x86_64": ("script", "scripts/ci_checks/{script}.sh"),
    "linux-arm":    ("script", "scripts/ci_checks/{script}.sh"),
    "linux-mips":   ("script", "scripts/ci_checks/{script}.sh"),
    "linux-checks": ("script", "scripts/ci_checks/{script}.sh"),
    "macos":        ("script", "scripts/ci_checks/macos/{script}.sh"),
    # android jobs don't key off matrix.script; their step command is
    # hard-coded in build.yml. We assert the script exists directly
    # below.
}


class BuildWorkflowTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.workflow = _load_workflow()

    # ---- matrix / script consistency ---------------------------------

    def test_every_matrix_entry_has_matching_script(self):
        """Each matrix.include entry must point at an existing script.

        Issue: the workflow uses `${{ matrix.script }}` to interpolate
        the script path. If the matrix and scripts/ci_checks/ drift,
        the job runs against a non-existent path and fails with a
        confusing "No such file or directory" on the runner.
        """
        jobs = self.workflow.get("jobs", {})
        failures = []
        for job_id, (key, template) in JOB_SCRIPT_PATHS.items():
            job = jobs.get(job_id)
            if job is None:
                failures.append(f"job '{job_id}' missing from workflow")
                continue
            matrix = (job.get("strategy") or {}).get("matrix") or {}
            entries = matrix.get("include") or []
            if not entries:
                failures.append(f"job '{job_id}' has no matrix.include")
                continue
            for entry in entries:
                if key not in entry:
                    failures.append(
                        f"job '{job_id}' matrix entry missing '{key}': {entry}"
                    )
                    continue
                script_value = entry[key]
                path = os.path.join(REPO_ROOT, template.format(script=script_value))
                if not os.path.isfile(path):
                    failures.append(
                        f"job '{job_id}' matrix.{key}={script_value!r} "
                        f"resolves to missing file: {os.path.relpath(path, REPO_ROOT)}"
                    )
        self.assertFalse(failures, "matrix/script drift:\n  " + "\n  ".join(failures))

    def test_android_jobs_reference_real_scripts(self):
        """The android jobs hard-code their script paths; check them too.

        android-linux: scripts/ci_checks/docker.sh <image> scripts/ci_checks/android/linux.sh
        android-macos: scripts/ci_checks/android/macos.sh
        """
        jobs = self.workflow.get("jobs", {})
        for job_id, relpath in [
            ("android-linux", "scripts/ci_checks/android/linux.sh"),
            ("android-macos", "scripts/ci_checks/android/macos.sh"),
        ]:
            self.assertIn(job_id, jobs, f"job '{job_id}' missing")
            self.assertTrue(
                os.path.isfile(os.path.join(REPO_ROOT, relpath)),
                f"job '{job_id}' references missing script: {relpath}",
            )

    def test_no_duplicate_resolved_job_names_within_a_job(self):
        """Two matrix entries with the same resolved job name are indistinguishable
        in the GitHub Actions UI.

        Issue: a job's display name is rendered from its `name` template
        (e.g. `${{ matrix.script }}` or `macos${{ matrix.macos-version }}-...`).
        If two matrix entries render to the same string, the UI shows them
        as one job and logs become confusing.
        """
        jobs = self.workflow.get("jobs", {})
        for job_id in JOB_SCRIPT_PATHS:
            job = jobs.get(job_id) or {}
            template = str(job.get("name") or job_id)
            entries = (job.get("strategy") or {}).get("matrix", {}).get("include") or []

            def render(tpl, entry):
                # Substitute ${{ matrix.<key> }} placeholders. Anything
                # else is left as-is (GitHub's own context expressions
                # cannot be evaluated statically). Keys may contain
                # hyphens, so use a character class instead of \w.
                def repl(m):
                    key = m.group(1)
                    return str(entry.get(key, m.group(0)))
                return re.sub(r"\$\{\{\s*matrix\.([\w-]+)\s*\}\}", repl, tpl)

            seen = {}
            for i, entry in enumerate(entries):
                resolved = render(template, entry)
                if resolved in seen:
                    self.fail(
                        f"job '{job_id}' produces duplicate job name "
                        f"{resolved!r} from matrix entries {seen[resolved]} and {i}: "
                        f"{entry}"
                    )
                seen[resolved] = i

    # ---- concurrency.group -------------------------------------------

    def test_concurrency_group_disambiguates_non_pr_triggers(self):
        """concurrency.group must not collapse all non-PR runs into one slot.

        Issue: the current expression is
            build-${{ github.event.pull_request.number || github.ref }}
        For push / repository_dispatch / workflow_dispatch / schedule,
        pull_request.number is empty, so every run on the same ref
        shares the group. Combined with cancel-in-progress: true, a
        later push cancels the earlier one mid-flight.

        We assert the expression references at least one discriminator
        that differs across non-PR trigger types, e.g. github.event_name
        or github.workflow + github.ref.
        """
        conc = self.workflow.get("concurrency") or {}
        group = conc.get("group") or ""
        self.assertTrue(group.strip(), "concurrency.group is empty")

        # GitHub Actions context expressions look like ${{ ... }}.
        # Keys may contain hyphens (e.g. matrix.macos-version), so use
        # a character class instead of \w.
        exprs = re.findall(r"\$\{\{\s*([^}]+?)\s*\}\}", group)

        # Concatenate all subexpressions so we can search the union.
        joined = " ".join(exprs)

        non_pr_discriminators = (
            "github.event_name",
            "github.workflow",
            "github.run_id",
            "github.run_number",
            "github.ref_name",
        )
        has_discriminator = any(d in joined for d in non_pr_discriminators)

        # If the expression only mentions pull_request.number and
        # github.ref, every non-PR run on the same ref collapses into
        # one group -- that is the bug we are guarding against.
        only_pr_and_ref = (
            "github.event.pull_request.number" in joined
            and "github.ref" in joined
            and not has_discriminator
        )
        self.assertFalse(
            only_pr_and_ref,
            "concurrency.group collapses non-PR runs onto a single group; "
            "add a discriminator such as github.event_name, github.workflow, "
            "or github.run_id so pushes, schedule, and dispatch do not cancel "
            "each other.",
        )

    # ---- structural sanity -------------------------------------------

    def test_every_job_has_runs_on(self):
        jobs = self.workflow.get("jobs") or {}
        missing = [jid for jid, j in jobs.items() if "runs-on" not in j]
        self.assertFalse(missing, f"jobs missing 'runs-on': {missing}")

    def test_every_job_has_steps(self):
        jobs = self.workflow.get("jobs") or {}
        for jid, job in jobs.items():
            self.assertIn(
                "steps", job,
                f"job '{jid}' has no 'steps' (and does not use a reusable workflow)",
            )
            self.assertTrue(
                job["steps"], f"job '{jid}' has empty 'steps'"
            )

    def test_top_level_keys(self):
        wf = self.workflow
        self.assertIn("name", wf, "workflow missing top-level key 'name'")
        self.assertIn("jobs", wf, "workflow missing top-level key 'jobs'")
        # The 'on' key may be parsed as the YAML 1.1 boolean True; the
        # _on_key helper handles both spellings.
        self.assertIsNotNone(
            _on_key(wf), "workflow missing top-level 'on' trigger spec"
        )


if __name__ == "__main__":  # pragma: no cover
    unittest.main()