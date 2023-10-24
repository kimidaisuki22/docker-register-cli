#!/bin/bash

# Define the remote and its URL
remote_name="template"
remote_url="https://github.com/kimidaisuki22/cpp-template.git"

# Check if the remote already exists
if ! git remote | grep -q "$remote_name"; then
    # Add the remote if it doesn't exist
    git remote add "$remote_name" "$remote_url"
else
    echo "Remote '$remote_name' already exists."
fi

# Fetch from all remotes
git fetch --all

# Merge the branch, allowing unrelated histories
git merge "$remote_name/main" --allow-unrelated-histories
