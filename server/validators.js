"use strict";

const NAME_PATTERN = /^[A-Za-z][A-Za-z .'-]{0,48}$/;

function isRecord(value) {
  return value !== null && typeof value === "object" && !Array.isArray(value);
}

function positiveInt(value, label = "id", max = 1000000) {
  const parsed = typeof value === "string" && /^\d+$/.test(value)
    ? Number(value) : value;
  if (!Number.isSafeInteger(parsed) || parsed < 1 || parsed > max) {
    const error = new Error(`${label} must be an integer from 1 to ${max}`);
    error.status = 400;
    throw error;
  }
  return parsed;
}

function requiredName(value, label = "name") {
  if (typeof value !== "string" || !NAME_PATTERN.test(value.trim())) {
    const error = new Error(`${label} must be 1-49 letters, spaces, apostrophes, periods, or hyphens`);
    error.status = 400;
    throw error;
  }
  return value.trim();
}

function requiredCity(value) {
  if (typeof value !== "string" || value.trim().length < 1 ||
      value.trim().length > 39 || /[,\u0000-\u001f]/.test(value)) {
    const error = new Error("city must be 1-39 characters and cannot contain commas or control characters");
    error.status = 400;
    throw error;
  }
  return value.trim();
}

function ageValue(value) {
  if (!Number.isInteger(value) || value < 1 || value > 120) {
    const error = new Error("age must be an integer from 1 to 120");
    error.status = 400;
    throw error;
  }
  return value;
}

function topK(value = "5") {
  const parsed = typeof value === "string" && /^\d+$/.test(value) ? Number(value) : value;
  if (!Number.isSafeInteger(parsed) || parsed < 1 || parsed > 100000) {
    const error = new Error("k must be an integer from 1 to 100000");
    error.status = 400;
    throw error;
  }
  return parsed;
}

function userFields(body) {
  if (!isRecord(body)) {
    const error = new Error("JSON object body required");
    error.status = 400;
    throw error;
  }
  return {
    id: positiveInt(body.id),
    name: requiredName(body.name),
    age: ageValue(body.age),
    city: requiredCity(body.city)
  };
}

module.exports = { isRecord, positiveInt, requiredName, requiredCity, ageValue, topK, userFields };
