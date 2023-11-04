#! /usr/bin/env python3
from colorama import Fore, Style
import argparse
import colorama
import functools
import json
import os
import os.path
import re
import subprocess
import sys

DRY_RUN = False

def error(message):
    print(f'{Fore.RED}{Style.BRIGHT}error:{Style.RESET_ALL} {message}', file=sys.stderr)
    sys.exit(1)

def run_cmd(cmd, input=None, stdout=None, env=None):
    cmd = [str(c) for c in cmd]
    pretty = ' '.join(['"'+c+'"' if ' ' in c else c for c in cmd])
    print(f'{Fore.YELLOW}{pretty}{Style.RESET_ALL}')
    if input:
        input = input.encode()
    if env:
        e = os.environ.copy()
        e.update(env)
        env = e
    try:
        if not DRY_RUN:
            subprocess.run(cmd, input=input, stdout=stdout, env=env, check=True)
    except subprocess.CalledProcessError as e:
        error('Command failed')

def verify_clean():
    if not os.path.isdir('.git'):
        error("Bad invocation: should be in repo root")

    if os.path.exists('.git/index.lock'):
        error("Bad invocation: git operation is in progress")

    if subprocess.run(['git', 'status', '--porcelain'],
                      stdout=subprocess.PIPE).stdout.decode() != '':
        error("Bad invocation: working copy is not clean")

def remember_ref():
    try:
        ref = subprocess.run(
            ['git', 'symbolic-ref', '--short', 'HEAD'],
            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, check=True).stdout
    except:
        ref = subprocess.run(
            ['git', 'rev-parse', 'HEAD'],
            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, check=True).stdout
    return ref.decode().strip()

def restore_ref(ref):
    if os.path.exists('.git/index.lock'):
        run_cmd(['git', 'rebase', '--abort'])

    run_cmd(['git', 'checkout', ref])

def delete_ref(ref):
    run_cmd(['git', 'branch', '-D', ref])

def guess_issue(org, repo, pr_body):
    if not pr_body:
        return None

    m = re.search(r'(?:^|\s)(#|gh-)(\d+)(?:$|\s)', pr_body, re.IGNORECASE)
    if m:
        return org, repo, m.group(2)

    for m in re.finditer(
        r'(?:^|\s)([a-zA-Z0-9_-]+)/([a-zA-Z0-9_-]+)#(\d+)(?:$|\s)',
        pr_body, re.IGNORECASE):
        if m.group(1) == org:
            return org, m.group(2), m.group(3)

    for m in re.finditer(
        r'(?:^|\s)https?://github.com/([a-zA-Z0-9_-]+)/([a-zA-Z0-9_-]+)/issues/(\d+)(?:$|\s)',
        pr_body, re.IGNORECASE):
        if m.group(1) == org:
            return org, m.group(2), m.group(3)

    return None

def make_prefix(org, repo, pr_info):
    issue_org, issue_repo, issue_number = pr_info['issue_link']

    if issue_org == org and issue_repo == repo:
        return f'gh-{issue_number}'
    else:
        return f'{issue_org}/{issue_repo}#{issue_number}'

@functools.lru_cache
def query_pr(org, repo, pr_number):
    result = subprocess.run(['gh', 'api', f'/repos/{org}/{repo}/pulls/{pr_number}'],
        capture_output=True, text=True)
    if result.returncode != 0:
        error('Failed to retrieve pr info')

    pr_json = json.loads(result.stdout)

    pr_info = {
        'pr_link': (org, repo, pr_number),
        'pr_title': pr_json['title'],
        'pr_url': pr_json['html_url'],
        'pr_state': pr_json['state'],
        'pr_draft': pr_json['draft'],
        'source_branch': pr_json['head']['ref'],
        'source_sha': pr_json['head']['sha'],
        'target_branch': pr_json['base']['ref'],
        'target_sha': pr_json['base']['sha'],
    }

    if 'issue_link' in pr_json:
        pr_info['issue_link'] = (org, repo, pr_json['issue']['number'])
    else:
        pr_info['issue_link'] = guess_issue(org, repo, pr_json['body'])

    if pr_info['issue_link']:
        issue_org, issue_repo, issue_number = pr_info['issue_link']

        result = subprocess.run([
            'gh', 'api',
            f'/repos/{issue_org}/{issue_repo}/issues/{issue_number}'
            ],
            capture_output=True, text=True)

        if result.returncode != 0:
            error('Failed to retrieve issue info')

        issue_json = json.loads(result.stdout)

        pr_info['issue_title'] = issue_json['title']
        pr_info['issue_url'] = issue_json['html_url']

    return pr_info

def show_pr(org, repo, pr_number):
    pr_info = query_pr(org, repo, pr_number)

    def section(name):
        print(f'{Fore.GREEN}{Style.BRIGHT}{name}:{Style.RESET_ALL}')

    def field(key, val, color=None):
        if color:
            print(f'  {key}: {color}{Style.BRIGHT}{val}{Style.RESET_ALL}')
        else:
            print(f'  {key}: {val}')

    section('pull request')
    field('title', pr_info['pr_title'], Fore.BLUE)
    field('url', pr_info['pr_url'])
    field('source', pr_info['source_branch'], Fore.CYAN)
    field('target', pr_info['target_branch'], Fore.CYAN)
    field('state', pr_info['pr_state'], Fore.MAGENTA)
    field('draft', str(pr_info['pr_draft']).lower(), Fore.MAGENTA)

    section('issue')
    if pr_info['issue_link']:
        field('title', pr_info['issue_title'], Fore.BLUE)
        field('url', pr_info['issue_url'])
    else:
        print('  none')

def verify_pr(org, repo, pr_number):
    pr_info = query_pr(org, repo, pr_number)

    if not pr_info['issue_link']:
        error("Can't determine issue associated with pr")

    if pr_info['pr_state'] != 'open':
        error("Can't proceed on non-open pr")

    if pr_info['pr_draft']:
        error("Can't proceed on draft pr")

def checkout_pr(org, repo, pr_number):
    run_cmd([
        'gh', 'pr', 'checkout',
        '--repo', f'{org}/{repo}',
        '-f',
        pr_number,
        ])

def reword_pr(org, repo, pr_number):
    pr_info = query_pr(org, repo, pr_number)

    commit_prefix = make_prefix(org, repo, pr_info)
    target_sha = pr_info['target_sha']

    run_cmd([
        'git', 'filter-branch', '-f', '--msg-filter',
        f"sed -r -e '1s,^(gh-[0-9]+ +|{org}/[^ ]+ +)?,{commit_prefix} ,'"+
          " -e '1s,\s*\(#[0-9]+\)$,,'",
        f'{target_sha}..HEAD',
        ],
        env={'FILTER_BRANCH_SQUELCH_WARNING':'1'})

def rebase_pr(org, repo, pr_number, remote):
    pr_info = query_pr(org, repo, pr_number)

    branch = pr_info['target_branch']

    run_cmd(['git', 'fetch', '-v', remote, branch])

    run_cmd([
        'git', 'rebase', f'{remote}/{branch}',
        ])

def squash_pr(org, repo, pr_number, remote):
    pr_info = query_pr(org, repo, pr_number)

    branch = pr_info['target_branch']
    message = make_prefix(org, repo, pr_info) + ' ' + re.sub(
        r'\s*\(#\d+\)$', '', pr_info['pr_title'])

    run_cmd(['git', 'fetch', '-v', remote, branch])

    run_cmd([
        'git', 'rebase', '-i', f'{remote}/{branch}',
        ],
        env={
            'GIT_EDITOR': ':',
            'GIT_SEQUENCE_EDITOR': "sed -i '1 ! s,^pick,fixup,g'",
        })

    run_cmd([
        'git', 'commit', '--amend', '--no-edit', '-m', message
        ])

def merge_pr(org, repo, pr_number):
    run_cmd([
        'gh', 'pr', 'merge',
        '--repo', f'{org}/{repo}',
        '--rebase',
        '--delete-branch',
        pr_number,
        ])

def push_pr():
    run_cmd(['git', 'push', '-f'])

parser = argparse.ArgumentParser(prog='pr.py')

parser.add_argument('--org', default='roc-streaming', help='github org')
parser.add_argument('--repo', default='roc-toolkit', help='github repo')
parser.add_argument('--remote', default='origin', help='remote name of github repo')
parser.add_argument('-l,--local', action='store_true', dest='local',
                    help='skip actual changes on remote repo')
parser.add_argument('-n,--dry-run', action='store_true', dest='dry_run',
                    help='print commands, but do not execute them')

subparsers = parser.add_subparsers(dest='command')

status_parser = subparsers.add_parser('show')
status_parser.add_argument('pr_number', type=int)

reword_parser = subparsers.add_parser('reword')
reword_parser.add_argument('pr_number', type=int)

merge_parser = subparsers.add_parser('merge')
merge_parser.add_argument('--rebase', action='store_true',
                          help='merge using rebase')
merge_parser.add_argument('--squash', action='store_true',
                          help='merge using squash')
merge_parser.add_argument('pr_number', type=int)

args = parser.parse_args()

DRY_RUN = args.dry_run

colorama.init()

if args.command == 'show':
    show_pr(args.org, args.repo, args.pr_number)
    exit(0)

if args.command == 'reword':
    verify_clean()
    verify_pr(args.org, args.repo, args.pr_number)
    orig_ref = remember_ref()
    try:
        checkout_pr(args.org, args.repo, args.pr_number)
        reword_pr(args.org, args.repo, args.pr_number)
        if not args.local:
            push_pr()
    finally:
        restore_ref(orig_ref)
    exit(0)

if args.command == 'merge':
    if int(bool(args.rebase)) + int(bool(args.squash)) != 1:
        error("Either --rebase or --squash should be specified")
    verify_clean()
    verify_pr(args.org, args.repo, args.pr_number)
    orig_ref = remember_ref()
    pr_ref = None
    merged = False
    try:
        checkout_pr(args.org, args.repo, args.pr_number)
        pr_ref = remember_ref()
        if args.rebase:
            rebase_pr(args.org, args.repo, args.pr_number, args.remote)
            reword_pr(args.org, args.repo, args.pr_number)
        else:
            squash_pr(args.org, args.repo, args.pr_number, args.remote)
        if not args.local:
            push_pr()
            merge_pr(args.org, args.repo, args.pr_number)
            merged = True
    finally:
        restore_ref(orig_ref)
        if merged:
            delete_ref(pr_ref)
    exit(0)
