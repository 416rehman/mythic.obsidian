# Obsidian Vault

This repository contains the structure of how I organize my notes, drawings, kanboards, and canvases in [Obsidian](https://obsidian.md/). I use Obsidian to take notes on a variety of topics and for project management.

The following directories are used in this repository:

  * `.obsidian (default)` - This directory contains the configured plugins and settings for Obsidian.
  * `_obsidian` - This directory contains other folders that are used by Obsidian.
  * `_obsidian/_attachments` - This directory contains all the attachments that are linked to in the notes.
  * `_obsidian/_templates` - This directory contains templates that are used by different plugins, such as Excalidraw and Kanban.
  * `Drawings`* - This directory contains all the drawings that are created in Excalidraw.
  * `Notes` - This directory contains all the markdown notes.
  * `Boards` - This directory is for all the kanban boards and canvases.

**Note:** Each excalidraw `.md` file automatically creates a corresponding `.svg` file that is then embedded into the excalidraw `.md` file so the drawn contents are visible natively when the `.md` file is rendered outside Obsidian (such as in Github). 

## How to use this repository
This vault is designed to be used as a Wiki so all the notes can be viewed in the browser through Github Wiki from your repository.

1. Create a new repository by clicking `Use this template` and clone the newly-created repository.
2. Go to the Wiki tab in your repo, and initialize a new blank page, and copy the Wiki GIT url (bottom right in Wiki tab).
3. Open a terminal in the cloned repo directory, and run `git remote set-url origin <your wiki git url>`, then run the following 
```bash
git fetch # Get the branches of the WIKI (default: master)
git pull --allow-unrelated-histories # Get any files/changes from the wiki
git push # Push everything to the wiki
```
4. Open the repo folder as a vault in Obsidian by clicking on `Open folder as vault` in Obsidian, and confirm any prompts.
5. Enjoy! Write notes, they will automatically sync to your wiki and you can view them in the browser.
