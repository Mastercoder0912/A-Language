const fs = require("fs");
const path = require("path");

const root = path.resolve(__dirname, "..");
const requiredJson = [
  "package.json",
  "language-configuration.json",
  "syntaxes/a.tmLanguage.json",
];

function readJson(relativePath) {
  const fullPath = path.join(root, relativePath);
  try {
    return JSON.parse(fs.readFileSync(fullPath, "utf8"));
  } catch (error) {
    throw new Error(`${relativePath}: ${error.message}`);
  }
}

for (const file of requiredJson) {
  readJson(file);
}

const manifest = readJson("package.json");
const languages = manifest.contributes && manifest.contributes.languages;
const grammars = manifest.contributes && manifest.contributes.grammars;

if (!Array.isArray(languages) || languages.length === 0) {
  throw new Error("package.json must contribute at least one language.");
}

if (!Array.isArray(grammars) || grammars.length === 0) {
  throw new Error("package.json must contribute at least one grammar.");
}

for (const language of languages) {
  for (const iconPath of Object.values(language.icon || {})) {
    const fullPath = path.join(root, iconPath);
    if (!fs.existsSync(fullPath)) {
      throw new Error(`Missing language icon: ${iconPath}`);
    }
  }

  if (language.configuration && !fs.existsSync(path.join(root, language.configuration))) {
    throw new Error(`Missing language configuration: ${language.configuration}`);
  }
}

for (const grammar of grammars) {
  if (!fs.existsSync(path.join(root, grammar.path))) {
    throw new Error(`Missing grammar file: ${grammar.path}`);
  }
}

console.log("A Language VS Code extension validation passed.");
